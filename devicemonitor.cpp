#include "devicemonitor.h"
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QDateTime>
#include <QCoreApplication>  // QCoreApplication::applicationDirPath()
#include <QDir>              // QDir
#include <QFile>             // QFile
#include <QDateTime>         // QDateTime
#include <QJsonDocument>     // QJsonDocument
#include <QJsonObject>       // QJsonObject
#include <QVariantMap>       // QVariantMap
#include <QTcpSocket>        // QTcpSocket
#include <QDebug>            // qDebug(), qWarning()
#include "global.h"
#include "DeviceAPI.h"
#include <QTcpSocket>


extern "C" {
#include "DeviceAPI.h"   // 只引入 API，不再直接 include Motor/Gpio/Proc_fw
#include "log.h"
}
DeviceMonitor::DeviceMonitor(MqttHelper* mqtt,QStackedWidget *qstack,QObject* parent)
    : QObject(parent), mqttHelper(mqtt),stack(qstack)
{

    startTime = QDateTime::currentDateTime();

    // // 連接 1 確認變化後在發訊息給MQTT，不要持續發送
    QMetaObject::Connection connection1 = connect(&monitorTimer, &QTimer::timeout, this, &DeviceMonitor::checkStatus);
    if (!connection1) {
        qDebug() << "❌ Connect failed for checkStatus!";
    } else {
        qDebug() << "✅ Connect successful for checkStatus!";
    }

    // 連接 2
    QMetaObject::Connection connection2 = connect(&monitorTimer, &QTimer::timeout, this, &DeviceMonitor::checkDeviceStatus);
    if (!connection2) {
        qDebug() << "❌ Connect failed for checkDeviceStatus!";
    } else {
        qDebug() << "✅ Connect successful for checkDeviceStatus!";
    }

    //雷射異常自動排除
    // 連接 3
    recoveryTimer.setSingleShot(true); //timer只執行一次
    QMetaObject::Connection connection3 = connect(&recoveryTimer, &QTimer::timeout, this, &DeviceMonitor::onRecoveryTimeout);
    // 連接 4
    // recoveryStopTimer.setSingleShot(true);
    QMetaObject::Connection connection4 = connect(&recoveryStopTimer, &QTimer::timeout, this, &DeviceMonitor::onRecoveryStopTimeout);

    // 連接 5  每10分鐘再次上報
    QMetaObject::Connection connection5 = connect(&statusUpdateTimer, &QTimer::timeout, this, &DeviceMonitor::updateStatus);
    if (!connection3) {
        qDebug() << "❌ Connect5 failed for checkDeviceStatus!";
    } else {
        qDebug() << "✅ Connect5 successful for checkDeviceStatus!";
    }

    // connect(mqttHelper->m_client, &QMqttClient::connected, this, &DeviceMonitor::onMqttConnected);

    QTimer::singleShot(10000, this, [this] {
        onMqttConnected(); //延遲 10 秒 發送開機訊息
    });

    // 原本的發送開機訊息
    // connect(mqttHelper->m_client, &QMqttClient::connected, this, [this] {
    //     QTimer::singleShot(12000, this, [this] {
    //         onMqttConnected();
    //     });
    // });

}

DeviceMonitor::~DeviceMonitor()
{
    delete test_err_dlg;
}

void DeviceMonitor::updateStatus()
{
    sendTelemetry(lastCode48);
}

void DeviceMonitor::startMonitoring(int intervalMs)
{
    monitorTimer.start(intervalMs);
    statusUpdateTimer.start( 10 * 60 * 1000); //10分鐘上報
    qDebug() << "✅ DeviceMonitor started, interval:" << intervalMs / 1000 << "seconds";

    // 立即檢查 monitorTimer 是否 active
    if (monitorTimer.isActive()) {
        qDebug() << "✅ monitorTimer is actively counting down.";
    } else {
        qDebug() << "❌ monitorTimer is NOT active. Check for potential issues.";
    }
}

double DeviceMonitor::getCpuTemperature()
{
    QFile file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "⚠️ 無法讀取 CPU 溫度";
        return -1.0;
    }
    QByteArray line = file.readAll().trimmed();
    bool ok;
    double temp = line.toDouble(&ok);
    return ok ? temp / 1000.0 : -1.0; // 攝氏
}

double DeviceMonitor::getCpuUsage()
{
    QProcess process;
    process.start("bash", QStringList() << "-c" <<
                              "grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage}'");
    process.waitForFinished();
    QByteArray output = process.readAllStandardOutput().trimmed();
    bool ok;
    double usage = output.toDouble(&ok);
    return ok ? usage : -1.0;
}

double DeviceMonitor::getMemoryUsage()
{
    QProcess process;
    process.start("bash", QStringList() << "-c" <<
                              "free | awk '/Mem/ {print $3/$2 * 100.0}'");
    process.waitForFinished();
    QByteArray output = process.readAllStandardOutput().trimmed();
    bool ok;
    double usage = output.toDouble(&ok);
    return ok ? usage : -1.0;
}

bool DeviceMonitor::isDisplayConnected()
{
    // QProcess process;
    // process.start("bash", QStringList() << "-c" <<
    //                           "(command -v vcgencmd >/dev/null && vcgencmd display_power | grep -q 'display_power=1') "
    //                           "|| (command -v tvservice >/dev/null && tvservice -s | grep -q 'HDMI') && echo 1 || echo 0");
    // process.waitForFinished();
    // QByteArray output = process.readAllStandardOutput().trimmed();
    // // return output == "1";
    // return output.contains("display_power=1");

    QProcess process;

    // 先試 vcgencmd
    process.start("vcgencmd", QStringList() << "display_power");
    if (process.waitForFinished(500)) {
        QByteArray out = process.readAllStandardOutput().trimmed();
        if (out.contains("display_power=1")) return true;
        if (out.contains("display_power=0")) return false;
    }

    // 再試 tvservice
    process.start("tvservice", QStringList() << "-s");
    if (process.waitForFinished(500)) {
        QByteArray out = process.readAllStandardOutput().trimmed();
        if (out.contains("HDMI")) return true;
        if (out.contains("disconnected") || out.contains("off")) return false;
    }

    // 如果都判斷不了，就當成 false
    return false;
}


// void DeviceMonitor::checkStatus()
// {
//     // handleAlert(checkPumpCurrent(), "PUMP_OVER_CURRENT", "ALARM", "泵浦電流異常");
//     // checkTankEmpty();
//     // checkWasteTankFull();
//     // // checkNetwork();
//     // handleAlert(checkDoorOpened(), "DOOR_OPENED", "WARN", "機門被打開");
//     // // handleAlert(checkScreenSignal(), "SCREEN_NO_SIGNAL", "ALARM", "螢幕無輸出");
//     // handleAlert(checkFlowWhilePumping(), "NO_FLOW_WHILE_PUMPING", "ALARM", "泵浦輸出但流量計 5 秒無訊息");

//     // if (checkCpuTemperature()) {
//     //     QVariantMap data{{"cpuTemp", getCpuTemperature()}};
//     //     handleAlert(true, "CPU_OVER_TEMPERATURE", "ALARM", "CPU 過溫", data);
//     // } else {
//     //     handleAlert(false, "CPU_OVER_TEMPERATURE", "ALARM", "CPU 過溫");
//     // }

//     // handleAlert(checkMemory(), "MEMORY_ANOMALY", "WARN", "RAM 可用不足或 OOM");
//     // handleAlert(checkMcuLink(), "MCU_LINK_ERROR", "ALARM", "STM32 通訊異常");
//     // handleAlert(checkDetergentExpiring(), "DETERGENT_EXPIRING_SOON", "WARN", "洗劑有效期剩餘 ≤ 5 日");
//     // handleAlert(checkCoinFull(), "COIN_FULL", "WARN", "計數1000"); //add in 2025/09/09 Mike
// }

void DeviceMonitor::checkStatus()
{

    const int PAGE_HOME     = 0;
    const int PAGE_LOGIN    = 1;
    const int PAGE_MEMBER   = 2; // 會員投放
    const int PAGE_GUEST    = 3; // 非會員投放
    const int PAGE_DONE_M   = 4; // 會員完成
    const int PAGE_DONE_G   = 5; // 非會員完成
    const int PAGE_AD       = 6;
    const int PAGE_ERROR    = 7;
    const int PAGE_MAINT_LOGIN = 8;
    const int PAGE_MAINT    = 9;

    auto cur = stack->currentIndex();
    qDebug()<<"機台自檢當前頁面為"<<cur;


    QString curr = lastCode48;

    //當雷射自我排除時每分鐘機台監測檢測暫時失效
    if(!m_isRecovering)
    {
        curr = computeCode48();
    }

    // const QString curr = computeCode48();

    if (curr != lastCode48) {
        sendTelemetry(curr);
        lastCode48 = curr;
    }

    QString status = decideStatus(lastCode48);

    // 這些頁面「不打斷」
    const QSet<int> protectedPages = { PAGE_MEMBER, PAGE_GUEST, PAGE_DONE_M, PAGE_DONE_G, PAGE_MAINT_LOGIN,PAGE_MAINT };

    if (status == "down" && !global::isMaintenanceMode) {
        // 只有當前不在保護頁，且也不是已在異常頁時，才跳轉
        if (!protectedPages.contains(cur) && cur != PAGE_ERROR) {
            // global::isStopMode = true;
            // LedAPI_Set(0x00ff00);//red 顯示紅燈
            LedAPI_Select(1);//red 顯示紅燈
            qDebug("該亮紅燈了吧");
            // if(global::sleepEnable){Relay1API_Off();}//RELAY 1 頭頂燈關閉
            if (global::idlewatcher()) global::idlewatcher()->pause();//暫停監控
            stack->setCurrentIndex(PAGE_ERROR);
            // if (global::getVideoShow()) global::getVideoShow()->hide();
            // if (global::getAdShow())    global::getAdShow()->stop();
            if (auto *v = global::getVideoShow(); v && v->isVisible())
                v->hide();
        }
    }
    // 從異常恢復：僅當目前在異常頁才回首頁與恢復播放
    else if (status != "down" && cur == PAGE_ERROR) {
        // LedAPI_Set(0xff0000);//green 顯示綠燈
        LedAPI_Select(2);//green 顯示綠燈
        // Relay1API_On();      //RELAY 1 頭頂燈打開
        stack->setCurrentIndex(PAGE_HOME);
        if (global::idlewatcher()) global::idlewatcher()->resume();//恢復減控
        // if (global::getVideoShow()) global::getVideoShow()->show();
        // if (global::getAdShow())    global::getAdShow()->start();
        if (auto *v = global::getVideoShow(); v && !v->isVisible())
            v->show();
        // global::isStopMode = false;
    }

    // if(status =="down" && global::isMaintenanceMode == false)
    // {
    //     if(stack->currentIndex()==0 || stack->currentIndex()==1 || stack->currentIndex()==6)
    //     {
    //         global::isStopMode = true;
    //         stack->setCurrentIndex(7);//異常頁面
    //         // global::getVideoShow()->stop();// 1. 暫停並隱藏影片輪播
    //         global::getVideoShow()->hide();
    //         global::getAdShow()->stop();//廣告暫停
    //     }
    // }else if(status !="down" && stack->currentIndex()==7){ //7異常頁面
    //     stack->setCurrentIndex(0);
    //     // global::getVideoShow()->start();// 1. 暫停並隱藏影片輪播
    //     global::getVideoShow()->show();
    //     global::isStopMode = false;
    // }
    // // 其它本地 UI 動作可自理（例如某些 bit 打開就切錯誤頁）
}

void DeviceMonitor::handleAlert(bool condition, const QString &code,
                                const QString &level, const QString &message,
                                const QVariantMap &data)
{
    // if (condition) {
    //     if (!activeAlerts.contains(code)) {
    //         activeAlerts.insert(code);
    //         sendAlert(code, level, message, data);
    //         qDebug() << "🚨 Alert triggered:" << code;
    //     }
    // } else {
    //     if (activeAlerts.contains(code)) {
    //         activeAlerts.remove(code);
    //         qDebug() << "✅ Alert cleared:" << code;
    //         // 這裡可以選擇要不要發一個 "clear" 訊息
    //         // sendAlert(code, "INFO", QString("%1 已恢復正常").arg(code));
    //     }
    // }
}

// ---------------------------------
// 以下為各種檢查函式 (暫時範例)
// ---------------------------------




bool DeviceMonitor::checkNetwork()
{
    bool networkDisconnected = false;

//     // ====== Step 1: Ping 公共網路，確認外網可通 ======
//     QProcess pingProcess;
// #ifdef Q_OS_WIN
//     pingProcess.start("ping", QStringList() << "-n" << "1" << "8.8.8.8");
// #else
//     pingProcess.start("ping", QStringList() << "-c" << "1" << "8.8.8.8");
// #endif

//     if (!pingProcess.waitForFinished(3000)) {
//         qWarning() << "[Network] Ping timeout: 無法完成 ping 命令";
//         networkDisconnected = true;
//     } else if (pingProcess.exitCode() != 0) {
//         qWarning() << "[Network] Ping failed: 無法連線到 8.8.8.8";
//         networkDisconnected = true;
//     }

//     if (networkDisconnected) {
//         if (mqttHelper && mqttHelper->m_client) {
//             mqttHelper->disconnectBroker();
//             qInfo() << "[Network] MQTT broker 已強制斷線";
//         }
//     }
//     // //網路連接 mqtt 不連接 做 mqtt 連線
//     // else if(!networkDisconnected && mqttHelper->m_client->state() != QMqttClient::Connected)
//     // {
//     //     mqttHelper->connectBroker();
//     // }

    QTcpSocket socket;
    const QString host = "8.8.8.8"; // 公網 IP，Google DNS
    const quint16 port = 53;        // DNS TCP port

    socket.connectToHost(host, port);

    // 等待連線，設定 timeout 2 秒
    bool connected = socket.waitForConnected(2000);

    if (connected) {
        //         qDebug() << "[Network] Internet reachable via" << host << ":" << port;
        socket.disconnectFromHost();
        networkDisconnected = false;
    } else {
        qWarning() << "[Network] Cannot reach internet via" << host << ":" << port;
        log_write("[Network] Cannot reach internet via");
        networkDisconnected = true;
    }

    return networkDisconnected;
}


// ----- 定時 1 分鐘回傳 -----
void DeviceMonitor::checkDeviceStatus()
{
    if (!mqttHelper) return;
    endTime = QDateTime::currentDateTime();
    QJsonObject payload;

    QDateTime now = QDateTime::currentDateTime();
    QString iso = now.toString(Qt::ISODate);  // "2025-09-11T14:32:10"

    int offsetSecs = now.offsetFromUtc();
    int offsetHours = offsetSecs / 3600;
    int offsetMinutes = (qAbs(offsetSecs) % 3600) / 60;

    // 格式化時區 (+08:00 / -05:30 ...)
    QString tz = QString("%1%2:%3")
                     .arg(offsetSecs >= 0 ? "+" : "-")
                     .arg(qAbs(offsetHours), 2, 10, QChar('0'))
                     .arg(offsetMinutes, 2, 10, QChar('0'));

    // 拼接成完整 ISO8601
    payload["ts"] = iso + tz;

    // payload["ts"] = QDateTime::currentDateTime().toString(Qt::ISODateWithOffset);
    // payload["deviceSerial"] = mqttHelper->deviceSerial; // 假設 deviceSerial 存在 MqttHelper
    // payload["cpuTempC"] = getCpuTemperature();
    // payload["cpuUsage"] = getCpuUsage();
    // payload["memUsagePercent"] = getMemoryUsage();
    // payload["display_connected"] = isDisplayConnected();
    // payload["fwVersion"] = "1.0.0";
    // payload["counter"] = startTime.msecsTo(endTime)/1000;
    // endTime = QDateTime::currentDateTime();
    mqttHelper->publishDeviceStatus(payload);
}

void DeviceMonitor::sendTelemetry(const QString& code)
{
    if (!mqttHelper) return;

    for (QChar ch : code) {
        if (ch == '1')
            alertCount++;
    }

    const QString level   = summarizeLevel(code);           // "Alarm"/"Warn"/"Info"
    const QString message = summarizeMessage(code);         // 英文 ≤30 bytes
    const QString count   = QString::number(alertCount);                           // 你要的兩位數字字串；可依需求帶變更數
    const QString data    = "";                             // 需要時放簡短描述（≤30 ASCII）



    QString status = decideStatus(code);
    //異常狀態顯示ui程式 移到外面
    // if(status =="down" && global::isMaintenanceMode == false)
    // {
    //     if(stack->currentIndex()==0 || stack->currentIndex()==1 || stack->currentIndex()==6)
    //     {
    //         global::isStopMode = true;
    //         stack->setCurrentIndex(7);//異常頁面
    //         // global::getVideoShow()->stop();// 1. 暫停並隱藏影片輪播
    //         global::getVideoShow()->hide();
    //         global::getAdShow()->stop();//廣告暫停
    //     }
    // }else if(status !="down" && stack->currentIndex()==7){ //7異常頁面
    //     stack->setCurrentIndex(0);
    //     // global::getVideoShow()->start();// 1. 暫停並隱藏影片輪播
    //     global::getVideoShow()->show();
    //     global::isStopMode = false;
    // }


    if (!mqttHelper) return;
    endTime = QDateTime::currentDateTime();
    // QJsonObject payload;

    QDateTime now = QDateTime::currentDateTime();
    QString iso = now.toString(Qt::ISODate);  // "2025-09-11T14:32:10"

    int offsetSecs = now.offsetFromUtc();
    int offsetHours = offsetSecs / 3600;
    int offsetMinutes = (qAbs(offsetSecs) % 3600) / 60;

    // 格式化時區 (+08:00 / -05:30 ...)
    QString tz = QString("%1%2:%3")
                     .arg(offsetSecs >= 0 ? "+" : "-")
                     .arg(qAbs(offsetHours), 2, 10, QChar('0'))
                     .arg(offsetMinutes, 2, 10, QChar('0'));


    QJsonObject p;
    // p["deviceSerial"] = mqttHelper->deviceSerial;
    p["status"]       = status;
    p["level"]        = level;
    p["code"]         = code;                               // 48 chars '0'/'1'
    p["message"]      = message.left(30);
    p["count"]        = count;
    p["data"]         = data;
    p["ts"]           = iso + tz; // 拼接成完整 ISO8601
    // p["ts"]           = QDateTime::currentDateTime().toString(Qt::ISODateWithOffset);
    mqttHelper->publishTelemetry(p);
    qDebug() << "📤 telemetry changed:" << code << message << level;
}

QString DeviceMonitor::summarizeLevel(const QString& code)
{
    bool hasWarn = false;
    for (const auto& a : kAlarms) {
        if (a.bit < 0 || a.bit >= 48) continue;
        int i = idxForBit(a.bit);
        if (i < 0 || i >= code.size()) continue;

        if (code[i] == '1') {
            // if (QString::fromLatin1(a.level) == "Alarm") return "Alarm";
            // if (QString::fromLatin1(a.level) == "Warn")  hasWarn = true;
            if (a.level == "Alarm") return "Alarm";
            if (a.level == "Warn")  hasWarn = true;
        }
    }
    return hasWarn ? "Warn" : "Info";
}

QString DeviceMonitor::summarizeMessage(const QString& code)
{
    QString allMsg="";
    // 優先：Alarm → Warn → Info；挑第一個命中的 msg
    for (const char* targetLevel : { "Alarm", "Warn", "Info" }) {
        for (const auto& a : kAlarms) {
            // if (QString::fromLatin1(a.level) != targetLevel) continue;
            if (a.level != targetLevel) continue;
            if (a.bit < 0 || a.bit >= 48) continue;
            int i = idxForBit(a.bit);
            if (i < 0 || i >= code.size()) continue;

            if (code[i] == '1')
                // return QString::fromLatin1(a.msg); // ≤30 bytes
                allMsg += a.msg+" ";
                // return a.msg; // ≤30 bytes
        }
    }
    if(allMsg!="")
    {
        return allMsg;
    }
    return "OK";
}

QString DeviceMonitor::computeCode48()
{
    QString code(48, '0');

    auto setBit = [&](int b, bool on){
        if (b < 0 || b >= 48) return;
        if (on) code[idxForBit(b)] = '1';
    };



    setBit(44, checkNetwork());// 網路斷線 把mqtt手動斷線  故意寫反的
    setBit(0, mqttHelper->m_client->state() != QMqttClient::Connected);// Mqtt離線
    setBit(1, global::isStopMode);//暫停模式

    // CPU/記憶體
    const double cpuC = getCpuTemperature();
    const double memP = getMemoryUsage();
    setBit(2, cpuC >= 80.0);   // 你現在用 40°C（原規劃是 80°C，請確認）
    setBit(3, memP >= 85.0); //

    setBit(4, global::isLoginfalse); // 登入失敗
    // setBit(5, false); //
    setBit(6, global::isPhishing); // 釣魚3次

    setBit(7, global::MotorStalled); //馬達堵轉


    //DeviceAPI_GetMotorErrorCode() 回傳之訊息
    const int mErr = DeviceAPI_GetMotorErrorCode();
    // bool motorFault = (global::motorstatus != 0); // 預設 false
    bool motorFault = (global::motorstatus == 1 || global::motorstatus == 2);
    // islaserFault 雷射異常旗標
    bool islaserFault = false;

    switch (mErr) {
    case 7:  setBit(8,  true); islaserFault = true; break;   // 入口雷射
    case 8:  setBit(10, true); islaserFault = true; break;   // 長度雷射
    case 9:  setBit(11, true); islaserFault = true; break;   // 高度雷射
    case 10: setBit(9,  true); islaserFault = true; break;   // 金屬感測
    case 11: case 12: case 13: case 14: case 15: case 16:
        motorFault = true;              // 標記馬達群組錯誤
        break;
    default:
        break;
    }

    //待測試
    if(!islaserFault) m_isLocked = false;//雷射無異常解鎖狀態
    recoveryCheckLaser(islaserFault);

    // if(islaserFault && m_failCount<3)
    // {
    //     recoveryTimer.start(3 * 60 * 1000);
    // }
    // else
    // {
    //     recoveryTimer.stop();
    // }

    // 統一在最後設定馬達感測 bit
    if (motorFault)
        setBit(16, true);


    //單個雷射接收異常
    switch (global::motorstatus) {
    case 4:  setBit(17,  true); break;   // 入口雷射
    case 5:  setBit(18, true); break;   // 長度雷射
    case 6:  setBit(19, true); break;   // 高度雷射
    default:
        break;
    }

    //休眠設定
    global::sleepEnable = isInSleepPeriod();//休眠檢測
    if(global::sleepEnable)
    {
        auto *page = stack->widget(7);   // 取得 index = 7 的頁面
        if (auto *err = qobject_cast<ErrorItem_Dialog*>(page)) {
            err->showSleep();
        }
    }
    else
    {
        auto *page = stack->widget(7);   // 取得 index = 7 的頁面
        if (auto *err = qobject_cast<ErrorItem_Dialog*>(page)) {
            err->showMaintain();
        }
    }
    setBit(25, global::sleepEnable); // 機台休眠中

    setBit(26, global::estWeightG ()>= 32000); // 重量已滿

    setBit(45, global::isMaintenanceMode); // 工程模式



    // 工程模式中（bit31）
    // const bool isMaintenanceMode = AppState::instance().isMaintenanceMode ;
    // setBit(31, isMaintenanceMode);


    //需要移到發布函式內計算
    // for (QChar ch : code) {
    //     if (ch == '1')
    //         alertCount++;
    // }
    return code;
}

QString DeviceMonitor::decideStatus(const QString& code)
{
    // for (const auto& a : kAlarms) {
    //     if (a.bit < 0 || a.bit >= 48) continue;    // 邏輯 bit 編號上限
    //     int i = idxForBit(a.bit);                  // 轉換成字串 index
    //     if (i < 0 || i >= code.size()) continue;   // 保護

    //     if (code[i] != '1') continue;

    //     if (a.level.compare("Alarm", Qt::CaseInsensitive) == 0) {
    //         return "down";   // 有任何 Alarm → down
    //     }
    //     // Warn 一律忽略
    // }
    // return "up";

    int emptyCount = 0;  // 計算桶 A~D 缺液數量

    for (const auto& a : kAlarms) {
        if (a.bit < 0 || a.bit >= 48) continue;
        int i = idxForBit(a.bit);
        if (i < 0 || i >= code.size()) continue;

        if (code[i] != '1') continue;

        // Alarm
        if (a.level.compare("Alarm", Qt::CaseInsensitive) == 0) {
            // 如果是桶 A/B/C/D 缺液，先計數，不立即 down
            if (a.msg == "EMPTY_A" ||
                a.msg == "EMPTY_B" ||
                a.msg == "EMPTY_C" ||
                a.msg == "EMPTY_D")
            {
                emptyCount++;
            }
            else {
                // 其他任何 Alarm，立刻 down
                return "down";
            }
        }
        // Warn 一律忽略
    }

    // 檢查缺液桶數量
    if (emptyCount >= 4) {
        return "down";   // 4個以上桶缺液 → down
    }

    return "up";          // 其餘情況 → up
}

void DeviceMonitor::recount()
{
    alertCount = 0;
    lastCode48 = QString(48, '0');
    sendTelemetry(lastCode48);
}
//純發開機
void DeviceMonitor::onMqttConnected()
{
    // 檢查是否為第一次連線
    if (!hasPublishedOnStartup) {
        qDebug() << "MQTT 第一次連線成功，發布啟動訊息...";

        QDateTime now = QDateTime::currentDateTime();
        QString iso = now.toString(Qt::ISODate);
        int offsetSecs = now.offsetFromUtc();
        int offsetHours = offsetSecs / 3600;
        int offsetMinutes = (qAbs(offsetSecs) % 3600) / 60;
        QString tz = QString("%1%2:%3")
                         .arg(offsetSecs >= 0 ? "+" : "-")
                         .arg(qAbs(offsetHours), 2, 10, QChar('0'))
                         .arg(offsetMinutes, 2, 10, QChar('0'));

        //lastCode48[0] = '1'; 這個打開下面也要打開
        QString Code48 = QString(48, '0');
        Code48[0] = '1';

        QJsonObject p;
        p["status"] = "up";
        p["level"] = "Info";
        //p["code"] = lastCode48;
        p["code"] = Code48;
        p["message"] = "";
        p["count"] = QString::number(alertCount += 1); // 請確認 alertCount 的定義
        p["data"] = global::SWVersion;//軟韌體版本
        p["ts"] = iso + tz;
        mqttHelper->publishTelemetry(p);
        // lastCode48 = QString(48, '0'); 這個打開上面也要打開
        // 發送後將標誌設為 true，確保之後不會再次發送
        hasPublishedOnStartup = true;

    } else {
        qDebug() << "MQTT 重連成功，但已發布過啟動訊息，不重複發布。";
    }
}

//code固定  純發送關機狀態
void DeviceMonitor::sendMessage(const QString &status,
                                const QString &level,
                                const QString &code,
                                const QString &message,
                                const QString &count,
                                const QString &data)
{
    if (!mqttHelper) return;
    QDateTime now = QDateTime::currentDateTime();
    QString iso = now.toString(Qt::ISODate);
    int offsetSecs = now.offsetFromUtc();
    int offsetHours = offsetSecs / 3600;
    int offsetMinutes = (qAbs(offsetSecs) % 3600) / 60;
    QString tz = QString("%1%2:%3")
                     .arg(offsetSecs >= 0 ? "+" : "-")
                     .arg(qAbs(offsetHours), 2, 10, QChar('0'))
                     .arg(offsetMinutes, 2, 10, QChar('0'));

    QString Code48 = QString(48, '0');
    // Code48[47-46] = '1';
    Code48[1] = '1';
    QJsonObject p;
    p["status"] = status;
    p["level"] = level;
    p["code"] = Code48;
    p["message"] = message;
    p["count"] = QString::number(alertCount += 1); // 請確認 alertCount 的定義
    p["data"] = data;
    p["ts"] = iso + tz;
    mqttHelper->publishTelemetry(p);
    // 發送後將標誌設為 true，確保之後不會再次發送
    hasPublishedOnStartup = true;

}

bool DeviceMonitor::isInSleepPeriod()
{
    QTime now = QTime::currentTime();
    const QTime &start = QTime::fromString(global::readConfigValue("sleepStrart","00:00").toString(),"HH:mm");
    const QTime &end = QTime::fromString(global::readConfigValue("sleepEnd","00:00").toString(),"HH:mm");

    // start == end 可視為無休眠
    if (start == end)
        return false;

    if (start < end) {
        // 同一天區間，例如 00:30 ~ 08:00
        return (now >= start && now < end);
    } else {
        // 跨午夜，例如 20:30 ~ 08:00
        return (now >= start || now < end);
    }
}


void DeviceMonitor::recoveryCheckLaser(bool isError)
{
    if(m_isRecovering || m_isLocked)
    {
        qInfo() << "已連續三次異常，系統鎖定";
        return;
    }
    qInfo() << "雷射異常。";

    if(isError)
    {
        LedAPI_Select(1);//red 顯示紅燈

        m_failCount++;
        if (m_failCount > 3) {
            m_isLocked = true;
            qInfo() << "連續三次異常，系統鎖定。";
            recoveryTimer.stop();
            return;
        }
        // m_failCount++;
        m_isRecovering = true;
        // recoveryTimer.start(1 * 60 * 1000);//3分鐘執行一次
        recoveryTimer.start(5 * 1000);//5秒執行一次
    }
    else
    {
        m_failCount = 0;
    }
}

// 3分鐘到了，馬達啟動
void DeviceMonitor::onRecoveryTimeout()
{
    qInfo() << "等待結束，馬達運轉 12 秒...";
    DeviceAPI_Run_Stop();//異常後先關閉
    global::motorstatus = DeviceAPI_Run();//關閉後再開啟
    // qInfo() << "馬達執行中...";
    // m_isRecovering = false;

    m_sensorCheckCount = 0;
    recoveryStopTimer.start(500); // 0.5秒執行一次 12秒後停止
}



// 12秒到了，馬達停止
void DeviceMonitor::onRecoveryStopTimeout()
{
    m_sensorCheckCount++;
    if(m_sensorCheckCount>=24)//12秒每次0.5秒
    {
        recoveryStopTimer.stop();
        m_isRecovering = false;
        checkStatus();
        // recoveryTimer.start(10 * 1000);
    }
    else
    {
        if(DeviceAPI_GetMotorErrorCode()==0)
        {
            m_isRecovering = false;
            recoveryStopTimer.stop();
            DeviceAPI_Run_Stop();
            checkStatus();
        }
    }

    //========================================
    // qInfo() << "馬達運轉結束，恢復常規檢測。";
    // DeviceAPI_Run_Stop();
    // // 關鍵：解除旗標，讓繼續下一次判定
    // m_isRecovering = false;
    // checkStatus();
}
