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

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "alsa");
    QApplication a(argc, argv);

    // ✅ 註冊應用程式退出時要呼叫的清理程式
    qAddPostRoutine(cleanupResources);

    QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    for (int i = 0; i < outputs.size(); ++i) {
        const QAudioDevice &device = outputs.at(i);
        qDebug() << "Available audio device:" << device.description();
    }


    QProcess::execute("amixer", {"set", "Master", "100%", "unmute"});// 聲音開啟
    global::initConfig();//初始建立機台檔案已經有檔案就不會建立

    // global::writeConfig("normalShutdown", false);// 先改為false 正常關閉在改為true使下在開機判斷是否正常關閉
    global::deviceId  = global::readConfigValue("deviceId","0000").toString();//讀機台號
    global::estWeightG() = global::readConfigValue("totalWeight","0.0").toDouble();//讀累積重量
    qDebug()<<"開啟程式讀入當前值累積之重量"<<global::estWeightG();

    // MQTT
    // global::mqtt().init("FLCBRM3_2508001");
    global::mqtt().init(global::deviceId);
    global::mqtt().connectBroker();

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
    backstage *bk = new backstage(&stack);//工程頁面 8


    // stack.addWidget(&hm); //首頁 0
    // stack.addWidget(&mblg); //登錄頁 1
    // stack.addWidget(&pibp); //會員投放頁 2
    // stack.addWidget(&nmbpibp);//非會員投放頁 3
    // stack.addWidget(&mep); //會員完成頁 4
    // stack.addWidget(&nmep); //會員完成頁 5

    stack.addWidget(hm); //首頁 0
    stack.addWidget(mblg); //登錄頁 1
    stack.addWidget(pibp); //會員投放頁 2
    stack.addWidget(nmbpibp);//非會員投放頁 3
    stack.addWidget(mep); //會員完成頁 4
    stack.addWidget(nmep); //會員完成頁 5
    stack.addWidget(ad); //廣告輪播頁 6
    global::setAdShow(ad);//設定廣告輪播全域變數
    ad->setPlayPath("廣告輪播資料夾");
    ad->start();  //廣告播放
    stack.addWidget(err_dlg);//異常頁面 7
    stack.addWidget(bk);//工程頁面 8

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
    overlay->setPlayPath("輪播資料夾");
    overlay->raise();
    overlay->show(); // 先顯示，交給下面邏輯控制
    overlay->start(); //start 輪撥才會開始

    stack.setCurrentWidget(hm);     // 可選：預設顯示首頁
    stack.showFullScreen();     // 全螢幕模式
    stack.show();                     // ★ 一定要 show（或 showMaximized / showFullScreen）
    ClockBus::instance()->start();    // 啟動共用時鐘（放show前或後都行 但必須先建立視窗)

    // auto* idle = new IdleWatcher(1 * 60 * 1000); // 5 分鐘
    auto* idle = new IdleWatcher(1 * 60 * 1000, &stack); //  1鐘無操作進入待機
    idle->install();

    QObject::connect(idle, &IdleWatcher::becameIdle, [&stack,&ad,&overlay]{
        // 閒置時…
        // overlay->stop();//可刪除
        overlay->hide();
        stack.setCurrentWidget(ad);
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
    DeviceMonitor* monitor = new DeviceMonitor(&global::mqtt(), &stack, &stack);
    global::monitor() = monitor;
    // 開始每分鐘心跳偵測/系統異常
    monitor->startMonitoring();


    return a.exec();
}
