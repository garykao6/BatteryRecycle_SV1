#include "mqtthelper.h"
#include <QDateTime>
#include <QDebug>
#include <QProcessEnvironment>
#include "global.h"
#include "DeviceAPI.h"//之後用在不同程式可刪除
MqttHelper::MqttHelper(QObject *parent)
    : QObject(parent),
    m_client(new QMqttClient(this))
{
    // m_client->setHostname("mqtt-staging.ecoco.xyz");//測試環境
    // m_client->setPort(1883);
    // m_client->setUsername("ecoco");
    // m_client->setPassword("7xWD*o2kiN-v");

    m_client->setHostname("mqtt.funleadchange.com");//測試環境
    //m_client->setHostname("mqtt.ecoco.xyz"); //正式環境
    m_client->setPort(1883);
    m_client->setUsername("ecoco");
    m_client->setPassword("7xWD*o2kiN-v");

    //Mike 2025/09/22
    m_client->setCleanSession(false); // 2025/9/22 啟用持久會話
    m_client->setKeepAlive(60);//心跳設定

    // ✅ 建議：設定 Last Will（retain=true 方便雲端拉狀態）遺囑
    // m_client->setWillTopic(QString("device/%1/status").arg(m_deviceId));
    // m_client->setWillMessage("offline");
    // m_client->setWillQoS(1);
    // m_client->setWillRetain(true);

    reconnectTimer = new QTimer(this); //2025/9/22
    reconnectTimer->setSingleShot(true); // 設定為只發送一次訊號


    // 設定訊號槽
    connect(m_client, &QMqttClient::connected, this, &MqttHelper::onConnected);
    connect(m_client, &QMqttClient::disconnected, this, &MqttHelper::onDisconnected);
    connect(m_client, &QMqttClient::messageReceived, this, &MqttHelper::onMessageReceived);

    // ✅ 設定 timeout 才呼叫 connectToHost（真的延遲）
    connect(reconnectTimer, &QTimer::timeout, this, [this]{
        if (m_client->state() == QMqttClient::Disconnected) {
            qDebug() << "⛓️‍💥 重連中...";
            m_client->connectToHost();
        }
    });

}

MqttHelper::~MqttHelper() {
    disconnectBroker();
}

// 初始化連線
void MqttHelper::init(const QString &deviceId) {
    m_deviceId = deviceId;
}

// 連線 MQTT Broker
void MqttHelper::connectBroker() {

    // m_client->connectToHost();
    if (m_client->state() == QMqttClient::Disconnected)
        m_client->connectToHost();
}

// 斷線 MQTT Broker
void MqttHelper::disconnectBroker() {
    // if (m_client->state() == QMqttClient::Connected)
    //     m_client->disconnectFromHost();

    if (m_client->state() != QMqttClient::Disconnected) {
        reconnectTimer->stop();
        m_client->disconnectFromHost();
    }
}

// --- Topic Helpers ---
QString MqttHelper::topicEvent(const QString &name) const { return QString("device/%1/event/%2").arg(m_deviceId, name); }
QString MqttHelper::topicCmd(const QString &name) const { return QString("device/%1/cmd/%2").arg(m_deviceId, name); }
QString MqttHelper::topicCfg(const QString &name) const { return QString("device/%1/cfg/%2").arg(m_deviceId, name); }

// --- 發佈事件 ---
void MqttHelper::publishEvent(const QString &eventName, const QJsonObject &payload, int qos, bool retain) {
    QJsonDocument doc(payload);
    // m_client->publish(topicEvent(eventName), doc.toJson(QJsonDocument::Compact), qos, retain); //原本的
    if (!m_client || m_client->state() != QMqttClient::Connected) {
        qWarning() << "❌ MQTT 未連線，無法發佈訊息 已加入佇列" ;
        // 如果未連線，將訊息推入佇列
        messageQueue.enqueue({topicEvent(eventName), doc.toJson(QJsonDocument::Compact)});
        return;
    }
    m_client->publish(topicEvent(eventName), doc.toJson(QJsonDocument::Compact), 1, false);
}

// --- 發佈指令 ---
void MqttHelper::publishCmd(const QString &cmdName, const QJsonObject &payload, int qos, bool retain) {
    QJsonDocument doc(payload);
    // m_client->publish(topicCmd(cmdName), doc.toJson(QJsonDocument::Compact), qos, retain); //原本的
    m_client->publish(topicCmd(cmdName), doc.toJson(QJsonDocument::Compact), 1, false);
}

// --- 常用事件封裝 ---
// 登入事件
void MqttHelper::sendLoginEvent(const QString &method, const QString &phone) {
    // 先建立內層 login 物件
    QJsonObject loginObj;
    loginObj["method"] = method;
    loginObj["phone"] = phone;

    // 再建立外層 payload
    QJsonObject payload;
    payload["deviceId"] = m_deviceId;
    payload["ts"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    payload["login"] = loginObj;

    publishEvent("login", payload);
}

// 回收完成事件
// void MqttHelper::sendRecycleEvent(double estWeight_g, int points, bool donation,
//                                   const QMap<int,int> &batteryCount,  // 電池數量 map，例如 {3:1, 2:1}
//                                   const QString &loginMethod,         // 例如 "PHONE"
//                                   const QString &phoneNumber)         // 例如 "0912345678"
void MqttHelper::sendRecycleEvent(double estWeight_g, int points, bool donation,
                                  const QMap<QString,int> &batteryCount,  // 電池數量 map，例如 {3:1, 2:1}
                                  const QString &loginMethod,         // 例如 "PHONE"
                                  const QString &phoneNumber, // 例如 "0912345678"
                                  int reject) // 拒收次數
{
    // count 物件
    // QJsonObject countObj;
    // for (auto it = batteryCount.constBegin(); it != batteryCount.constEnd(); ++it) {
    //     countObj[QString::number(it.key())] = it.value();
    // }
    QJsonObject countObj;
    for (auto it = batteryCount.constBegin(); it != batteryCount.constEnd(); ++it) {
        countObj[it.key()] = it.value();
    }

    // points 物件
    // QJsonObject pointsObj{
    //     {"policyVer", "points-20250901"},
    //     {"totalByDevice", points}
    // };

    // txn 物件
    QJsonObject txn{
        {"estWeightG", estWeight_g}
    };

    // recycle 物件
    QJsonObject login{
        {"method", loginMethod},
        {"phone", phoneNumber},
        {"ts",global::loginTime}//登入時間
    };

    // payload
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"stage", "DONE"},
        {"accept", true},
        {"counts", countObj},
        {"txn", txn},
        {"points", points},
        {"donation", donation},
        // {"rejectCode", QJsonValue::Null},
        {"rejectCount", reject},
        {"login", login},
        {"totalWeightG",global::estWeightG()},// 總重
        {"stallCount",DeviceAPI_GetMotorStalled()},//馬達堵轉次數
        {"kgCO2e",global::carbonReduction()},     // 該次交易碳量
        {"deviceTotalKgCo2e", global::total_carbonReduction} // 機台累積總回收碳量
    };

    publishEvent("recycle", payload);
}


// 捐贈事件
void MqttHelper::sendDonationEvent(double estWeight_g,const QMap<QString,int> &batteryCount ,int reject , bool accept) {

    QJsonObject countObj;
    for (auto it = batteryCount.constBegin(); it != batteryCount.constEnd(); ++it) {
        countObj[it.key()] = it.value();
    }

    QJsonObject txn{
        {"estWeightG", estWeight_g}
    };

    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"counts",countObj},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"stage", "DONE"},
        {"accept", accept},
        {"txn", txn},
        {"donation", true},
        {"rejectCount", reject},
        {"totalWeightG",global::estWeightG()},// 總重
        {"stallCount",DeviceAPI_GetMotorStalled()},//馬達堵轉次數
        {"kgCO2e",global::carbonReduction()},     // 該次交易碳量,
        {"deviceTotalKgCo2e", global::total_carbonReduction} // 機台累積總回收碳量
    };

    publishEvent("donation", payload);
}

// Pi 狀態回報
void MqttHelper::sendPiStatusEvent(double cpuTemp, int memUsed, int memTotal, int uptimeMinutes, const QString &fwVersion, double binWeightKg) {
    QJsonObject memObj{
        {"usedMiB", memUsed},
        {"totalMiB", memTotal}
    };
    QJsonObject piObj{
        {"cpuTempC", cpuTemp},
        {"mem", memObj},
        {"lastResetTs", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"uptimeMinutes", uptimeMinutes},
        {"fwVersion", fwVersion}
    };
    QJsonObject binObj{
        {"estWeight_kg", binWeightKg}
    };
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"pi", piObj},
        {"bin", binObj}
    };
    publishEvent("piStatus", payload, 0, false);
}

// 警報事件
void MqttHelper::sendAlertEvent(const QString &alertCode, const QString &desc, const QJsonObject &metrics) {
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"alertCode", alertCode},
        {"desc", desc},
        {"metrics", metrics}
    };
    publishEvent("alert", payload);
}

// 心跳事件
void MqttHelper::sendHeartbeatEvent(const QJsonObject &status) {
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"status", status}
    };
    publishEvent("heartbeat", payload, 0);
}

// 系統日誌
void MqttHelper::sendSystemLog(const QString &level, const QString &message, const QJsonObject &data) {
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"logType", level},
        {"level", level},
        {"message", message},
        {"data", data}
    };
    publishEvent("system", payload, level=="ERROR"?1:0);
}

// OTA 更新進度
void MqttHelper::sendOtaProgress(const QString &refTs, const QString &phase, int percent) {
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"refTs", refTs},
        {"phase", phase},
        {"percent", percent}
    };
    publishEvent("otaProgress", payload);
}

// OTA 結果
void MqttHelper::sendOtaResult(const QString &refTs, const QString &result, const QJsonObject &detail, const QString &failReason) {
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"refTs", refTs},
        {"result", result},
        {"detail", detail},
        {"failReason", failReason.isEmpty() ? QJsonValue::Null : QJsonValue(failReason)}
    };
    publishEvent("otaResult", payload);
}

// 站點請求
void MqttHelper::sendstoreInfo() {
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)}
    };
    publishEvent("storeInfo", payload);
}

//anydesk id回傳
void MqttHelper::sendRequest(){
    QJsonObject data{
        {"anydeskId", global::anydeskId}
    };

    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"data",data}
    };
    publishEvent("syncRequest", payload);
}

void MqttHelper::sendcarouselAck(const QString taskId,const QString code,const QString note)
{
    QJsonObject data
    {
        {"code",code},
        {"note",""}
    };

    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)+"+08:00"},
        {"refTaskId",taskId},
        {"carouselAck",data}
    };
    publishEvent("carouselAck", payload);
}

void MqttHelper::sendpoweronDate()
{
    QJsonObject payload{
        {"deviceId", m_deviceId},
        {"ts", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)+"+08:00"},
        {"poweronDate",QDate::currentDate().toString("yyyy-MM-dd")},
    };
    publishEvent("poweronDate", payload);
}

// --- MQTT 連線/接收槽 ---
void MqttHelper::onConnected() {
    qDebug() << "MQTT Connected";
    if(!messageQueue.isEmpty())
    {
        qDebug()<<"斷線期間佇列內有資料";
        // 啟動逐筆送出
        if (!messageQueue.isEmpty())
            QTimer::singleShot(0, this, &MqttHelper::flushQueueSmoothly);
    }

    // ✅ 連上後：重置退避、取消計時器
    reconnectAttempts = 0;
    if (reconnectTimer->isActive()) reconnectTimer->stop();

    subscribeCmdTopics();  // 連線後訂閱指令
    subscribeCfgTopics();  // 連線後訂閱設定

}

void MqttHelper::onDisconnected() {
    qDebug() << "MQTT Disconnected";
    attemptReconnect(); // ✅ 觸發退避重連（不要直接 connectBroker
    // connectBroker();
}

void MqttHelper::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic) {
    QJsonDocument doc = QJsonDocument::fromJson(message);
    if (!doc.isObject()) return;
    QJsonObject payload = doc.object();

    QString t = topic.name();
    // 根據 topic 判斷發射哪個 signal
    if (t.endsWith("/loginAck")) emit loginAckReceived(payload);
    else if (t.endsWith("/pause")) emit pauseCmdReceived(payload);
    else if (t.endsWith("/resume")) emit resumeCmdReceived(payload);
    else if (t.endsWith("/reset")) emit resetCmdReceived(payload);
    else if (t.endsWith("/otaNotify")) emit otaNotifyReceived(payload);
    else if (t.endsWith("/otaCancel")) emit otaCancelReceived(payload);
    else if (t.endsWith("/setAutoShutdown")) emit setAutoShutdownReceived(payload);
    else if (t.endsWith("/setLightSchedule")) emit setLightScheduleReceived(payload);
    else if (t.endsWith("/setRingLight")) emit setRingLightReceived(payload);
    else if (t.endsWith("/syncResponse")) emit syncResponseReceived(payload);
    else if (t.endsWith("/setRules")) emit setRulesReceived(payload);
    else if (t.endsWith("/reboot")) emit rebootReceived(payload);
    else if (t.endsWith("/poweroff")) emit poweroffReceived(payload);
    else if (t.endsWith("/storeInfoAck")) emit storeInfoAckReceived(payload);
    else if (t.endsWith("/carouselNotify")) emit carouselNotifyReceived(payload);
    else if (t.endsWith("/sleep")) emit sleepCmdReceived(payload);
    else if (t.endsWith("/autoPoweroff")) emit autoPoweroffReceived(payload);
    else if (t.endsWith("/autoPoweroffCancel")) emit autoPoweroffCancelReceived(payload);
    else if (t.endsWith("/poweronDateAck")) emit poweronDateAckReceived(payload);

}

// --- 訂閱常用指令 ---
void MqttHelper::subscribeCmdTopics() {
    QStringList cmds = {
        "loginAck", "pause", "resume", "reset", "otaNotify", "otaCancel",
        "setAutoShutdown", "setLightSchedule", "setRingLight", "syncResponse",
        "reboot","poweroff","storeInfoAck","carouselNotify","sleep","autoPoweroff",
        "autoPoweroffCancel","poweronDateAck"
    };
    for (const QString &cmd : cmds)
    {
        m_client->subscribe(topicCmd(cmd), 2);
        qDebug()<<"MQtt 訂閱指令" <<cmd;
    }
}

// --- 訂閱設定相關 ---
void MqttHelper::subscribeCfgTopics() {
    m_client->subscribe(topicCfg("setRules"), 2);
    qDebug()<<"MQtt 訂閱設定" <<"setRules";
}

//MIke 2025/09/22
void MqttHelper::attemptReconnect()// mqtt重新連線
{
    // 若不是「確實已斷線」或已在等待下次重連，就不要重複排程
    if (m_client->state() != QMqttClient::Disconnected) return;
    if (reconnectTimer->isActive()) return;

    // 1s, 2s, 4s, 8s, 16s, 32s, ...（到 60s 封頂）
    int exp = qMin(reconnectAttempts*4, 5);             // 封頂在 2^5 = 32 倍
    int baseMs = 1000 << exp;                         // 位移寫法避免 qPow(double)
    int jitter = QRandomGenerator::global()->bounded(baseMs / 4 + 1); // 0~25% 抖動
    int delay = qMin(baseMs + jitter, 60000);         // 最多 60 秒

    ++reconnectAttempts;
    qDebug() << "⏳ 退避重連 in" << delay << "ms（第" << reconnectAttempts << "次）";

    reconnectTimer->start(delay);
}





void MqttHelper::publishMaintenanceCommand(const QString& opId16,
                                           const QString& operation20,
                                           const QString& tank,          // "A"|"B"|"C"|"D" 或空字串
                                           const QString& data,          // 可空
                                           const QString& liquidExpiry)  // "yyyy/MM/dd" 可空
{
    // if (!client || client->state() != QMqttClient::Connected) {
    //     qWarning() << "❌ MQTT 未連線，無法發佈 maintenance";
    //     return;
    // }

    // ---- 小工具（區域 lambda），保持函式單檔可貼用 ----
    auto asciiSanitize = [](const QString& s, int maxLen) {
        QString out; out.reserve(s.size());
        for (QChar ch : s) {
            ushort u = ch.unicode();
            if (u >= 0x20 && u <= 0x7E) out.append(ch);
            else if (u == '\n' || u == '\r' || u == '\t') out.append(' ');
        }
        if (out.size() > maxLen) out.truncate(maxLen);
        return out;
    };
    auto fixedAscii = [&](const QString& raw, int fixedLen) {
        QString s = asciiSanitize(raw, fixedLen);
        if (s.size() < fixedLen) s.append(QString(fixedLen - s.size(), QChar(' ')));
        return s; // 長度固定 == fixedLen
    };
    auto isoNowWithOffset = []() {
        QDateTime now = QDateTime::currentDateTime();
        QString iso = now.toString(Qt::ISODate); // yyyy-MM-ddTHH:mm:ss
        int off = now.offsetFromUtc();
        int h = off / 3600;
        int m = (qAbs(off) % 3600) / 60;
        QString tz = QString("%1%2:%3")
                         .arg(off >= 0 ? "+" : "-")
                         .arg(qAbs(h), 2, 10, QChar('0'))
                         .arg(m, 2, 10, QChar('0'));
        return iso + tz; // e.g. 2025-09-08T11:20:00+08:00
    };
    auto isValidTank = [](QString t) {
        t = t.trimmed().toUpper();
        return t.isEmpty() || t=="A" || t=="B" || t=="C" || t=="D";
    };

    // ---- 欄位處理 ----
    const QString opIdFixed  = fixedAscii(opId16, 16);
    const QString opFixed    = fixedAscii(operation20.trimmed(), 20);
    const QString tankNorm   = tank.trimmed().toUpper();
    const QString ts         = isoNowWithOffset();

    if (!isValidTank(tankNorm)) {
        qWarning() << "⚠️ tank 非法，必須為 A/B/C/D 或空：" << tank;
        // 規格允許空字串；非法就不帶 tank 欄位
    }

    // liquid_expiry 驗證 (yyyy/MM/dd)
    // QRegularExpression ymdRx(R"(^(?:\d{4})\/(?:0[1-9]|1[0-2])\/(?:0[1-9]|[12]\d|3[01])$)"); //日期+時間
    QRegularExpression ymdRx(R"(^\d{4}/(0[1-9]|1[0-2])/(0[1-9]|[12]\d|3[01])$)"); //日期

    // ---- 組 JSON ----
    QJsonObject p;
    // p["deviceSerial"] = asciiSanitize(deviceSerial, 64);
    // p["opId"]         = opIdFixed;     // 固定 16 bytes
    // p["operation"]    = opFixed;       // 固定 20 bytes

    p["deviceId"] = m_deviceId;
    p["opId"]         = opId16;     // 固定 16 bytes
    p["operation"]    = operation20;  // 固定 20 bytes

    if (!tankNorm.isEmpty())  p["tank"] = tankNorm;
    if (!data.isEmpty())      p["data"] = asciiSanitize(data, 64);
    p["ts"]           = ts;
    if (!liquidExpiry.isEmpty()) {
        if (ymdRx.match(liquidExpiry).hasMatch()) {
            p["liquid_expiry"] = liquidExpiry;
        } else {
            qWarning() << "⚠️ liquid_expiry 需為 yyyy/MM/dd：" << liquidExpiry;
        }
    }

    const QString topic = QStringLiteral("device/%1/mode/maintenance").arg(m_deviceId);
    const QByteArray body = QJsonDocument(p).toJson(QJsonDocument::Compact);

    if (!m_client || m_client->state() != QMqttClient::Connected) {
        qWarning() << "❌ MQTT 未連線，無法發佈 maintenance 訊息已加入佇列" ;
        // 如果未連線，將訊息推入佇列
        messageQueue.enqueue({topic, body});
        return;
    }

    const qint32 mid = m_client->publish(QMqttTopicName(topic), body, /*qos*/1, /*retain*/false);
    if (mid == -1) {
        qWarning() << "❌ 發佈失敗:" << topic << body;
        return;
    }
    qDebug() << "✅ 發佈 maintenance:" << topic << body;
    // return;

}

void MqttHelper::publishTelemetry(QJsonObject &payload)
{
    // if (!client || client->state() != QMqttClient::Connected) {
    //     qWarning() << "❌ MQTT 尚未連線，無法發佈 Telemetry";
    //     return;
    // }


    payload["deviceId"] = m_deviceId;   // 你的機台序號 (建議在 MqttHelper 裡存起來)


    QJsonDocument doc(payload);
    QByteArray data = doc.toJson(QJsonDocument::Compact);


    QString topic = QString("device/%1/telemetry").arg(m_deviceId);

    if (!m_client || m_client->state() != QMqttClient::Connected) {
        qWarning() << "❌ MQTT 未連線，無法發佈 Telemetry 訊息已加入佇列" ;
        // 如果未連線，將訊息推入佇列
        messageQueue.enqueue({topic, data});
        return;
    }

    // auto result = m_client->publish(QMqttTopicName(topic), data, 1, false);
    auto result = m_client->publish(QMqttTopicName(topic), data = doc.toJson(QJsonDocument::Compact), 1, false);
    if (result == -1) {
        qWarning() << "❌ 發佈 Telemetry 失敗:" << topic;
    } else {
        qDebug() << "✅ 已發佈 Telemetry:" << data;
    }
}

void MqttHelper::publishDeviceStatus(QJsonObject &payload)
{
    if (!m_client || m_deviceId.isEmpty()) {
        qWarning() << "❌ MQTT 未初始化或 deviceSerial 未設定";
        return;
    }

    payload["deviceId"] = m_deviceId;

    // // ---- 寫入每日 log 檔案 ----
    // QString logDir = QCoreApplication::applicationDirPath() + "/logs";
    // QDir().mkpath(logDir);  // 確保目錄存在

    // QString logFileName = logDir + "/device_status_" +
    //                       QDate::currentDate().toString("yyyyMMdd") + ".log";

    // QFile file(logFileName);
    // if (file.open(QIODevice::Append | QIODevice::Text)) {
    //     QJsonDocument logDoc(payload);
    //     file.write(logDoc.toJson(QJsonDocument::Compact));
    //     file.write("\n");  // 每筆一行
    //     file.close();
    //     qDebug() << "📝 已追加紀錄到:" << logFileName;
    // } else {
    //     qWarning() << "⚠️ 無法寫入檔案:" << logFileName;
    // }

    // ---- 發布到 MQTT ----
    QJsonDocument doc(payload);
    // QByteArray data = doc.toJson(QJsonDocument::Compact);
    QString topic = QString("device/%1/event/heartbeat").arg(m_deviceId);
    // auto result = m_client->publish(QMqttTopicName(topic), data, 1, false);
    auto result = m_client->publish(QMqttTopicName(topic), doc.toJson(QJsonDocument::Compact), 1, false);
    if (result == -1)
        qWarning() << "❌ 發佈 heartbeat 失敗";
    else
        qDebug() << "📤 發佈 heartbeat:" << doc.toJson(QJsonDocument::Compact);
}

void MqttHelper::flushQueueSmoothly()
{
    if (messageQueue.isEmpty() || m_client->state() != QMqttClient::Connected)
        return;

    const QPair<QString, QByteArray> msg = messageQueue.dequeue();
    auto pktId = m_client->publish(msg.first, msg.second, 1, false);
    if (pktId == -1) {
        // 發送失敗：丟回佇列，等下一次
        messageQueue.prepend(msg);
        qWarning() << "⚠️ publish failed, stop flush";
        return;
    }
    qDebug()<<"發佈"<<msg.first;

    // 若還有資料，排下一次
    if (!messageQueue.isEmpty()) {
        QTimer::singleShot(5, this, &MqttHelper::flushQueueSmoothly);
    }
}
