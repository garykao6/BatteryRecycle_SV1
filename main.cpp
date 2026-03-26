#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStackedWidget>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QDebug>
#include <QFontDatabase>
#include <QPixmapCache>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QLoggingCategory>

#include "homepage.h"
#include "clockbus.h"
#include "mb_loginpage.h"
#include "global.h"
#include "playtool.h"
#include "mb_putinbatterypage.h"
#include "mb_endpage.h"
#include "nmb_putinbatterypage.h"
#include "nmb_endpage.h"
#include "idlewatcher.h"
#include "mqtthelper.h"
#include "devicemonitor.h"
#include "erroritem_dialog.h"
#include <QProcess>
#include "backstage.h"
#include "backstagelogin_page.h"
#include "DeviceAPI.h"

#include <QSslSocket>

void cleanupResources()
{
    qDebug() << "[CleanUp] 開始釋放 Qt 靜態資源...";

    // 1. 釋放 font 資源
    QFontDatabase::removeAllApplicationFonts();

    // 2. 釋放圖片快取
    QPixmapCache::clear();
    QImageReader::setAllocationLimit(0);

    // 3. 強制清理 audio devices（僅列出避免懸掛，Qt 自動管理）
    const auto outputs = QMediaDevices::audioOutputs();
    for (const auto &device : outputs) {
        qDebug() << "  [Audio Device]:" << device.description();
    }

    qDebug() << "[CleanUp] Qt 靜態資源清理完成。";
}


void checkSslSupport()
{
    qDebug() << "-----------------------------------";
    qDebug() << "Qt SSL 支援檢查:";

    if (QSslSocket::supportsSsl()) {
        qDebug() << "✅ SSL 支援已啟用!";
        // 這些函數在 Qt 6 中仍然有效
        qDebug() << " - 函式庫版本:" << QSslSocket::sslLibraryBuildVersionString();
        qDebug() << " - 執行時版本:" << QSslSocket::sslLibraryVersionString();
    } else {
        qDebug() << "❌ SSL 支援未啟用。";

        // 刪除或註釋掉這一行，解決編譯錯誤
        // qDebug() << " - 原因:" << QSslSocket::sslLibraryErrorString();

        qDebug() << " (請確認您的 Qt 6 環境已正確部署 OpenSSL 函式庫，特別是在 Windows)";
    }
    qDebug() << "-----------------------------------";
}

void ClearFile(QString path)
{
    QDir dir(path);
    if (!dir.exists())
        return;

    // 刪除所有檔案
    QFileInfoList files = dir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
}

QByteArray fixLargeIntsInJson(const QByteArray& rawJson) {
    // 1. 定義安全整數限制 (2^53 = 9007199254740992)
    // 您的數字 (106768972971507710) 顯然超過此限制。

    // 2. 正規表達式：尋找 'taskId' 後面的數字
    // Regex 範例：尋找 "taskId": 後面的一組數字
    // ⚠️ 註：這是一個簡化範例，可能會匹配到其他地方的數字。
    // 在實際應用中，正規表達式需要非常精確，以確保只匹配 taskId。
    // 這裡我們假設 taskId 總是位於行首 (或您能定義更精確的模式)。

    QRegularExpression regex("\"taskId\":\\s*(\\d{18,})"); // 匹配 "taskId": 後面至少18位數

    QByteArray modifiedJson = rawJson;
    QRegularExpressionMatch match = regex.match(modifiedJson);

    if (match.hasMatch()) {
        QString originalNumber = match.captured(1);

        // 將匹配到的數字部分替換成字串格式
        // 從 "taskId": 1067...10 替換成 "taskId": "1067...10"
        modifiedJson.replace(
            match.capturedStart(1), // 數字開始的位置
            match.capturedLength(1), // 數字的長度
            ("\"" + originalNumber + "\"").toUtf8() // 替換為帶引號的字串
            );
        qDebug() << "JSON 預處理成功，taskId 已轉換為字串。";
    } else {
        qWarning() << "未找到匹配的 taskId，繼續使用原始 JSON。";
    }

    return modifiedJson;
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "alsa");

    // 把 qt.multimedia 的 log 關掉（放最前面）
    // QLoggingCategory::setFilterRules(QStringLiteral("qt.multimedia.*=false\n"));
    // 將 stderr 重定向到 /dev/null（完全不顯示 ffmpeg 的任何訊息）
    freopen("/dev/null", "w", stderr);

    QApplication a(argc, argv);

    // ✅ 註冊應用程式退出時要呼叫的清理程式
    qAddPostRoutine(cleanupResources);

    QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    for (int i = 0; i < outputs.size(); ++i) {
        const QAudioDevice &device = outputs.at(i);
        qDebug() << "Available audio device:" << device.description();
    }


    // QProcess::execute("amixer", {"set", "Master", "100%", "unmute"});// 聲音開啟及控制

    global::initConfig();//初始建立機台檔案已經有檔案就不會建立

    // global::writeConfig("normalShutdown", false);// 先改為false 正常關閉在改為true使下在開機判斷是否正常關閉
    global::deviceId  = global::readConfigValue("deviceId","0000").toString();//讀機台號
    global::estWeightG() = global::readConfigValue("totalWeight","0.0").toDouble();//讀累積重量
    qDebug()<<"開啟程式讀入當前值累積之重量"<<global::estWeightG();
    global::total_carbonReduction = global::readConfigValue("total_carbonReduction","0.0").toDouble();//讀累積機台碳足跡
    qDebug()<<"開啟程式讀入機台累積之碳足跡"<<global::total_carbonReduction;

    // MQTT
    // global::mqtt().init("FLCBRM3_2508001");
    global::mqtt().init(global::deviceId);
    global::mqtt().connectBroker();

    QApplication::setOverrideCursor(Qt::BlankCursor);//隱藏鼠標


    // 翻譯
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "flcbatteryV3_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    // global::videoShow().setGeometry(0,0,1080,1120);
    // global::videoShow().show();
    // global::videoShow().start();

    // video.show();
    // video.raise();
    // video.start();


    // 建議用自動生命期（不要 new），避免手動 delete
    QStackedWidget stack;
    // HomePage hm(&stack); //首頁 0
    // mb_LoginPage mblg(&stack);//登錄頁 1
    // mb_PutINBatteryPage pibp(&stack); //會員投放頁 2
    // nmb_PutInBatteryPage nmbpibp(&stack); //非會員投放頁 3
    // mb_EndPage mep(&stack);//會員完成頁 4
    // nmb_endpage nmep(&stack); //非會員完成頁 5

    HomePage *hm = new HomePage(&stack); //首頁 0
    mb_LoginPage *mblg = new mb_LoginPage(&stack);//登錄頁 1
    mb_PutINBatteryPage *pibp = new  mb_PutINBatteryPage(&stack); //會員投放頁 2
    nmb_PutInBatteryPage *nmbpibp = new nmb_PutInBatteryPage(&stack); //非會員投放頁 3
    mb_EndPage *mep = new mb_EndPage(&stack);//會員完成頁 4
    nmb_endpage *nmep = new nmb_endpage(&stack); //非會員完成頁 5
    PlayTool *ad = new PlayTool(&stack); //廣告輪播頁 6
    ErrorItem_Dialog *err_dlg = new ErrorItem_Dialog(&stack); //異常頁面 7
    err_dlg->setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框嵌入
    err_dlg->showMaintain();//工程模式
    backstageLogin_page *bklogin = new backstageLogin_page(&stack);//工程登入頁面8
    backstage *bk = new backstage(&stack);//工程頁面 9

    global::mqtt().sendstoreInfo();//發送取的站點名稱訊號



    // stack.addWidget(&hm); //首頁 0
    // stack.addWidget(&mblg); //登錄頁 1
    // stack.addWidget(&pibp); //會員投放頁 2
    // stack.addWidget(&nmbpibp);//非會員投放頁 3
    // stack.addWidget(&mep); //會員完成頁 4
    // stack.addWidget(&nmep); //會員完成頁 5

    stack.addWidget(hm); //首頁 0
    global::mqtt().sendstoreInfo();//發送取的站點名稱訊號

    stack.addWidget(mblg); //登錄頁 1
    stack.addWidget(pibp); //會員投放頁 2
    stack.addWidget(nmbpibp);//非會員投放頁 3
    stack.addWidget(mep); //會員完成頁 4
    stack.addWidget(nmep); //非會員完成頁 5
    stack.addWidget(ad); //廣告輪播頁 6
    global::setAdShow(ad);//設定廣告輪播全域變數
    ad->setPlayPath("idlepageAD");
    //ad->start();  //廣告播放
    ad->stop();
    stack.addWidget(err_dlg);//異常頁面 7
    stack.addWidget(bklogin);//工程登入頁面 8
    stack.addWidget(bk);//工程頁面 9

    global::soundPlayer();//建立全域播放器
    // global::playSound(); 填入播放檔案  EX:聲音資料夾名稱/檔名.mp3(聲音資料夾需與執行檔同目錄)

    // 建立影片視窗並掛在 stack 上方
    PlayTool *v_show = new PlayTool(&stack); //建立輪播物件
    global::setVideoShow(v_show);// 設置全域變數
    auto *overlay = global::getVideoShow(); //取得全域輪播變數

    // auto *overlay = &global::videoShow();//第一次呼叫時建立 並取得視窗指標
    overlay->setParent(&stack);
    overlay->setWindowFlags(Qt::Widget);
    overlay->setGeometry(0, 0, 1080, 1120);
    overlay->setPlayPath("homepageAd");
    overlay->raise();
    overlay->show(); // 先顯示，交給下面邏輯控制
    overlay->start(); //start 輪撥才會開始

    stack.setCurrentWidget(hm);     // 可選：預設顯示首頁
    stack.showFullScreen();     // 全螢幕模式
    stack.show();                     // ★ 一定要 show（或 showMaximized / showFullScreen）
    ClockBus::instance()->start();    // 啟動共用時鐘（放show前或後都行 但必須先建立視窗)

    // auto* idle = new IdleWatcher(1 * 60 * 1000); // 5 分鐘
    auto* idle = new IdleWatcher(1 * 60 * 1000, &stack); //  1鐘無操作進入待機
    global::idlewatcher() = idle;//待機檢測
    idle->install();

    QObject::connect(idle, &IdleWatcher::becameIdle, [&stack,&ad,&overlay]{
        // 閒置時…
        if (global::isStopMode) return;
        // overlay->stop();//可刪除
        // overlay->hide();
        stack.setCurrentWidget(ad);
        overlay->hide();
    });
    QObject::connect(idle, &IdleWatcher::userActivity, [&stack,&hm,&ad,&overlay]{
        // 操作時…
        global::playSound("sounds/歡迎語.mp3");
        stack.setCurrentWidget(hm);
        // overlay->start();
        overlay->show();
        // ad->start();
    });


    QObject::connect(pibp,&mb_PutINBatteryPage::idleWatchPoke,idle,&IdleWatcher::poke);
    QObject::connect(nmbpibp,&nmb_PutInBatteryPage::idleWatchPoke,idle,&IdleWatcher::poke);
    QObject::connect(bk,&backstage::idleWatchPoke,idle,&IdleWatcher::poke);

    QObject::connect(&global::mqtt(), &MqttHelper::pauseCmdReceived,[&stack,&err_dlg](){
        qDebug()<<"機台暫停";
        if (global::idlewatcher()) global::idlewatcher()->pause();//待機暫停監控
        global::getVideoShow()->stop();// 播放並顯示影片輪播
        global::getVideoShow()->hide();
        // LedAPI_Set(0x00ff00);//red
        LedAPI_Select(1);//紅燈
        global::isMaintenanceMode = true;//工程模式
        global::isStopMode = true;//暫停模式
        global::writeConfig("keepStop",global::isStopMode);//寫入檔案斷電維持暫停服務
        global::monitor()->checkStatus(); //馬上檢測
        stack.setCurrentWidget(err_dlg);//維護頁面
    });
    QObject::connect(&global::mqtt(), &MqttHelper::resumeCmdReceived,[&stack,&hm](){
        qDebug()<<"機台恢復";
        if (global::idlewatcher()) global::idlewatcher()->resume();//待機恢復監控
        global::getVideoShow()->start();// 播放並顯示影片輪播
        global::getVideoShow()->show();
        LedAPI_Select(2);//綠燈
        global::isMaintenanceMode = false;//工程模式
        global::isStopMode = false;//暫停模式
        global::writeConfig("keepStop",global::isStopMode);//寫入檔案斷電維持暫停服務
        global::monitor()->checkStatus(); //馬上檢測
        stack.setCurrentWidget(hm);//維護頁面
    });
    QObject::connect(&global::mqtt(), &MqttHelper::rebootReceived,[](){
        qDebug()<<"機台重啟";
        global::monitor()->sendMessage("down","Alarm","","重新啟動","",""); //調用檢查函式
        QCoreApplication::quit();
        QProcess::startDetached("sudo", {"reboot"});
    });
    QObject::connect(&global::mqtt(), &MqttHelper::poweroffReceived,[](){
        qDebug()<<"機台關機";
        global::monitor()->sendMessage("down","Alarm","","關機","",""); //調用檢查函式
        QCoreApplication::quit();
        QProcess::startDetached("sudo", {"poweroff"});
    });
    QObject::connect(&global::mqtt(), &MqttHelper::sleepCmdReceived,[](QJsonObject payload){
        const QString startStr = payload.value("start").toString();
        const QString endStr   = payload.value("end").toString();
        // global::writeConfig("sleepEnable",true); //寫入啟動休眠
        global::writeConfig("sleepStrart",startStr);//寫入起始休眠時間
        global::writeConfig("sleepEnd",endStr);//寫入結束休眠時間
        qDebug()<<"機台休眠設定";
        global::monitor()->checkStatus(); //馬上檢測
    });
    QObject::connect(&global::mqtt(), &MqttHelper::autoPoweroffReceived,[](QJsonObject payload){
        const QString autoPoweroffStr = payload.value("at").toString();
        global::writeConfig("powerOffTime",autoPoweroffStr);//寫入結束休眠時間
        qDebug()<<"機台自動關機設定";
    });
    QObject::connect(&global::mqtt(), &MqttHelper::autoPoweroffCancelReceived,[](QJsonObject payload){
        const QString autoPoweroffStr = "null";
        global::writeConfig("powerOffTime",autoPoweroffStr);//寫入關機時間
        qDebug()<<"關閉機台自動關機設定";
    });
    QObject::connect(&global::mqtt(), &MqttHelper::weightResetReceived,[](const QJsonObject &){
        qDebug()<<"遠端重量歸零";
        global::estWeightG() = 0;
        global::writeConfig("totalWeight", 0);
        if (global::monitor()) global::monitor()->checkStatus();
    });
    QObject::connect(&global::mqtt(), &MqttHelper::motorControlReceived,[&stack,&err_dlg](const QJsonObject &payload){
        qDebug()<<"後台馬達控制（先進入暫停再執行）";
        if (global::idlewatcher()) global::idlewatcher()->pause();
        global::getVideoShow()->stop();
        global::getVideoShow()->hide();
        // 二代為 LedAPI_Set(0x00ff00)；一代 API 僅 LedAPI_Select，與 resume 相同綠燈語意
        LedAPI_Select(2);
        global::isMaintenanceMode = true;
        global::isStopMode = true;
        global::writeConfig("keepStop", global::isStopMode);
        if (global::monitor()) global::monitor()->checkStatus();
        stack.setCurrentWidget(err_dlg);
        QString action = payload.value("action").toString();
        if (action == "forward") DeviceAPI_MotorForward();
        else if (action == "back") DeviceAPI_MotorBack();
        else if (action == "stop") DeviceAPI_MotorStop();
        if (global::monitor()) global::monitor()->checkStatus();
    });

    //收到廣告輪播資訊
    QObject::connect(&global::mqtt(), &MqttHelper::carouselNotifyReceived,[](const QJsonObject &payload){

        // QString savePath = QCoreApplication::applicationDirPath() + "/idlepageAD";
        // QString savePath = QCoreApplication::applicationDirPath() + "/homepageAd";

        // "solt":"HOME", // 首頁：HOME 待機：STANDBY
        QString slot = payload.value("solt").toString();
        QString savePath = QCoreApplication::applicationDirPath() + "/homepageAd";//預設
        if(slot == "HOME")
        {
            savePath = QCoreApplication::applicationDirPath() + "/homepageAd";
            ClearFile(savePath); //先清除裡面檔案
            qDebug()<<"路徑儲存路徑"<<"homepageAd";
        }
        else if(slot == "STANDBY")
        {
            savePath = QCoreApplication::applicationDirPath() + "/idlepageAD";
            ClearFile(savePath); //先清除裡面檔案
            qDebug()<<"路徑儲存路徑"<<"idlepageAD";
        }

        QDir().mkpath(savePath); // 確保資料夾存在

        // 取得基本資訊

        QString deviceId = payload.value("deviceId").toString();
        QString ts       = payload.value("ts").toString();
        QString taskId = payload.value("taskId").toString();
        QJsonArray list  = payload.value("list").toArray();

        qDebug() << "📢 收到輪播任務";
        qDebug() << "  deviceId:" << deviceId;
        qDebug() << "  taskId:" << taskId;
        qDebug() << "  項目數量:" << list.size();


        // 遍歷每個媒體項目  先把payload存成json檔 在讀進mediaitem
        static QNetworkAccessManager *mgr = new QNetworkAccessManager(qApp);
        int index = 1;

        for (const QJsonValue &v : list) {
            QJsonObject item = v.toObject();
            QString url      = item.value("url").toString();
            QString kind     = item.value("kind").toString();     // "image" or "video"
            QString name     = item.value("name").toString();
            qint64 size      = item.value("size").toInteger();
            QString hash     = item.value("hash").toString();
            int duration     = item.value("duration").toInt();

            qDebug().noquote() << "▶" << kind << name;
            qDebug().noquote() << "   URL:" << url;
            qDebug() << "   Size:" << size << "bytes";
            qDebug() << "   Duration:" << duration;
            qDebug() << "   SHA256:" << hash;

            QFileInfo nameInfo(name);
            QString base = nameInfo.completeBaseName();
            QString ext  = nameInfo.suffix();

            // 檔名格式：001_10s_原始檔名.ext
            QString localName = QString("%1_%2s_%3.%4")
                                    .arg(index, 3, 10, QLatin1Char('0'))  // 001, 002, ...
                                    .arg(duration)                        // 10s
                                    .arg(base)
                                    .arg(ext);

            index++; //index +1


            // 建立網路存取物件
            // QString filename = savePath + "/" + name;
            QString filename = savePath + "/" + localName;
            QUrl fileUrl(url);
            QNetworkRequest req(fileUrl);
            QNetworkReply *reply = mgr->get(req);

            // 非同步下載完成 callback
            QObject::connect(reply, &QNetworkReply::finished, [reply, filename]() {
                if (reply->error() != QNetworkReply::NoError) {
                    qWarning() << "❌ 下載失敗:" << reply->errorString();
                    reply->deleteLater();
                    return;
                }

                QByteArray data = reply->readAll();
                QFile f(filename);
                if (!f.open(QIODevice::WriteOnly)) {
                    qWarning() << "❌ 無法開啟檔案:" << filename;
                    reply->deleteLater();
                    return;
                }
                f.write(data);
                f.close();

                qDebug() << "✅ 下載完成:" << filename;
                reply->deleteLater();
            });
        }

        // ✅ 回覆 ACK 給伺服器
        global::mqtt().sendcarouselAck(taskId, "OK", "");
    });


    DeviceMonitor* monitor = new DeviceMonitor(&global::mqtt(), &stack, &stack);
    global::monitor() = monitor;
    // 開始每分鐘心跳偵測/系統異常
    monitor->startMonitoring();

    // 斷電復歸暫停頁判斷10秒後
    QTimer::singleShot(10000, &stack, [](){
        global::isStopMode = global::readConfigValue("keepStop",false).toBool();
        global::monitor()->checkStatus();
    });


    // 取得並回傳anydeskId
    QProcess process;
    process.start("anydesk", {"--get-id"});
    process.waitForFinished();
    global::anydeskId = process.readAllStandardOutput().trimmed();
    qDebug() << "📡 AnyDesk ID:" << global::anydeskId;
    global::mqtt().sendRequest();

    DeviceAPI_Open();//開啟API
    LedAPI_Select(2);//green 顯示綠燈
    //Relay1API_On(); //開頭頂燈
    // return a.exec();原本

    int ret = a.exec();   // 事件迴圈結束，程式要退出了
    DeviceAPI_Close();    // 在退出前釋放資源
    return ret;
}
