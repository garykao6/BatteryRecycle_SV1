#pragma once

#include <QObject>
#include <QMqttClient>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QRandomGenerator>
#include <QQueue>      // ← 必須：QQueue 的完整定義
#include <utility>
/*
 * MqttHelper
 * FLCB-V3 MQTT 通訊封裝
 * 支援事件發佈 (event)、指令接收 (cmd) 以及設定 (cfg)
 */
class MqttHelper : public QObject
{
    Q_OBJECT
public:
    explicit MqttHelper(QObject *parent = nullptr);
    ~MqttHelper();

    // 初始化 MQTT 連線設定
    void init(const QString &deviceId);

    // 連線/斷線 MQTT Broker
    void connectBroker();
    void disconnectBroker();

    // 通用發佈方法
    void publishEvent(const QString &eventName, const QJsonObject &payload, int qos = 1, bool retain = false);
    void publishCmd(const QString &cmdName, const QJsonObject &payload, int qos = 1, bool retain = false);

    // --- 常用事件封裝 ---
    void sendLoginEvent(const QString &method, const QString &phone);      // 登入事件
    // void sendRecycleEvent(double estWeight_g, int points, bool donation,
    //                       const QMap<int,int> &batteryCount,  // 電池數量 map，例如 {3:1, 2:1}
    //                       const QString &loginMethod,         // 例如 "PHONE"
    //                       const QString &phoneNumber); // 回收完成事件

    // void sendRecycleEvent(double estWeight_g, int points, bool donation,
    //                       const QMap<QString,int> &batteryCount,  // 電池數量 map，例如 {3:1, 2:1}
    //                       const QString &loginMethod,         // 例如 "PHONE"
    //                       const QString &phoneNumber); // 回收完成事件

    void sendRecycleEvent(double estWeight_g, int points, bool donation,
                          const QMap<QString,int> &batteryCount,  // 電池數量 map，例如 {3:1, 2:1}
                          const QString &loginMethod,         // 例如 "PHONE"
                          const QString &phoneNumber,
                          int reject); // 回收完成事件

    void sendDonationEvent(double estWeight_g, const QMap<QString,int> &batteryCount ,int reject,bool accept = true );        // 捐贈事件

    void sendPiStatusEvent(double cpuTemp, int memUsed, int memTotal, int uptimeMinutes, const QString &fwVersion, double binWeightKg); // Pi 狀態回報
    void sendAlertEvent(const QString &alertCode, const QString &desc, const QJsonObject &metrics); // 警報事件
    void sendHeartbeatEvent(const QJsonObject &status);                    // 心跳事件
    void sendSystemLog(const QString &level, const QString &message, const QJsonObject &data); // 系統日誌
    void sendOtaProgress(const QString &refTs, const QString &phase, int percent); // OTA 更新進度
    void sendOtaResult(const QString &refTs, const QString &result, const QJsonObject &detail, const QString &failReason = QString()); // OTA 結果
    void sendstoreInfo();//站點請求
    void sendRequest(); //目前用來回傳anydesk Id
    void sendcarouselAck(const QString taskId,const QString code,const QString note);//回傳輪播檔案是否ok
    void sendpoweronDate(); //發送合約開機

    //工程模式
    void publishMaintenanceCommand(const QString& opId16,
                                   const QString& operation20,
                                   const QString& tank,
                                   const QString& data,
                                   const QString& liquidExpiry);
    //機台狀態
    void publishTelemetry(QJsonObject &payload);

    //heart beat
    void publishDeviceStatus(QJsonObject &payload);



    QMqttClient *m_client;  // MQTT 客戶端
    QString m_deviceId;          // 裝置編號
    // QQueue<QPair<QString, QByteArray>> messageQueue;//訊息佇列
    QQueue<QPair<QString, QByteArray>> messageQueue;//MQTT訊息佇列

signals:
    // --- 接收到後台指令事件 ---
    void loginAckReceived(const QJsonObject &payload);         // 登入回覆
    void pauseCmdReceived(const QJsonObject &payload);        // 暫停指令
    void resumeCmdReceived(const QJsonObject &payload);       // 恢復指令
    void resetCmdReceived(const QJsonObject &payload);        // 重置指令
    void otaNotifyReceived(const QJsonObject &payload);       // OTA 通知
    void otaCancelReceived(const QJsonObject &payload);       // OTA 取消
    void setAutoShutdownReceived(const QJsonObject &payload); // 設定自動關機
    void setLightScheduleReceived(const QJsonObject &payload);// 設定燈光排程
    void setRingLightReceived(const QJsonObject &payload);    // 設定環形燈
    void syncResponseReceived(const QJsonObject &payload);    // 同步回覆
    void setRulesReceived(const QJsonObject &payload);        // 設定規則
    void rebootReceived(const QJsonObject &payload);          // 重啟
    void poweroffReceived(const QJsonObject &payload);        // 關機
    void storeInfoAckReceived(const QJsonObject &payload);    //站點名稱
    void carouselNotifyReceived(const QJsonObject &payload);  //廣告輪播
    void sleepCmdReceived(const QJsonObject &payload);        // 休眠指令
    void autoPoweroffReceived(const QJsonObject &payload);      //設定自動關機指令
    void autoPoweroffCancelReceived(const QJsonObject &payload);//關閉自動關機指令
    void poweronDateAckReceived(const QJsonObject &payload); //合約接收後台資訊
    void weightResetReceived(const QJsonObject &payload);    // 遠端電池重量歸零
    void motorControlReceived(const QJsonObject &payload);   // 後台馬達控制

private slots:
    void onConnected();          // MQTT 連線成功
    void onDisconnected();       // MQTT 斷線
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic); // 收到訊息

    // Mike 2025/09/22
    void attemptReconnect();// mqtt斷線重新連線
    void flushQueueSmoothly();

private:
    // QMqttClient *m_client;       // MQTT 客戶端
    // QString m_deviceId;          // 裝置編號

    // Topic 生成輔助
    QString topicEvent(const QString &name) const;
    QString topicCmd(const QString &name) const;
    QString topicCfg(const QString &name) const;

    // //工程模式
    // void publishMaintenanceCommand(const QString& opId16,
    //                                 const QString& operation20,
    //                                 const QString& tank,
    //                                 const QString& data,
    //                                 const QString& liquidExpiry);
    // //機台狀態
    // void publishTelemetry(QJsonObject &payload);

    // //heart beat
    // void publishDeviceStatus(QJsonObject &payload);

    void subscribeCmdTopics();   // 訂閱常用指令
    void subscribeCfgTopics();   // 訂閱設定相關指令

    int reconnectAttempts = 0; //重新連線次數
    QTimer *reconnectTimer = nullptr;//重新連線計時器
};
