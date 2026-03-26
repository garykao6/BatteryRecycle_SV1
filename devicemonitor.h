#pragma once
#include <QObject>
#include <QTimer>
#include "mqtthelper.h"
#include<QStackedWidget>
#include "erroritem_dialog.h"
class DeviceMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DeviceMonitor(MqttHelper* mqtt,QStackedWidget *qstack = nullptr, QObject* parent = nullptr);
    ~DeviceMonitor();

    void checkDeviceStatus();
    void checkStatus();
    void recount();
    // 偵測函式
    bool checkNetwork();//網路偵測
    // ErrorItem_Dialog *err_dlg = nullptr;

    //Mike
    // void trigetTelemetry(QString &status,QString &level,QString &code,QString &message,QString &count,QString &data);
    void setLastCodeBit(int b, bool on){
        if (b < 0 || b >= 48) return;
        if (on) lastCode48[idxForBit(b)] = '1';
    };
    void sendTelemetry(const QString& code);       // 組 payload 並 publish
    void sendMessage(const QString &status,const QString &level,const QString &code,const QString &message,const QString &count,const QString &data);

    QString lastCode48 = QString(48, '0');   // 上次已發布的 48bit 狀態碼

    double getCpuTemperature();
    double getCpuUsage();
    double getMemoryUsage();
    bool isDisplayConnected();
    bool isInSleepPeriod();//休眠判斷


public slots:
    void startMonitoring(int intervalMs = 60 * 1000); // 預設 1 分鐘

private:
    MqttHelper* mqttHelper;
    QStackedWidget *stack;
    QTimer monitorTimer;

    // 用來記錄已經發送過的異常
    QSet<QString> activeAlerts;

    // 管理 alert 發送與解除
    void handleAlert(bool condition, const QString &code,
                     const QString &level, const QString &message,
                     const QVariantMap &data = QVariantMap());

    //雷射異常自動排除功能
    void recoveryCheckLaser(bool isError);
    void onRecoveryTimeout(); //3分鐘異常恢復
    void onRecoveryStopTimeout();// 12秒到了，馬達停止

    void updateStatus();//每10分鐘再次上報狀態

    // 讀取機台狀態
    // double getCpuTemperature();
    // double getCpuUsage();
    // double getMemoryUsage();
    // bool isDisplayConnected();

    // 偵測函式
    // bool checkNetwork();

    // 封裝 alert 發送



    struct AlarmDef { int bit; const QString msg; const QString level; };
    // inline static const QVector<AlarmDef> kAlarms = {
    //     { 0,"FULL_EMPTY","Alarm"},
    //     { 1,"EMPTY_A","Warn"}, { 2,"FLOW_ERR_A","Alarm"}, { 3,"",""},
    //     { 4,"EMPTY_B","Warn"}, { 5,"",""}, { 6,"FLOW_ERR_B","Alarm"}, { 7,"",""},
    //     { 8,"EMPTY_C","Warn"}, { 9,"",""}, {10,"FLOW_ERR_C","Alarm"}, {11,"",""},
    //     {12,"EMPTY_D","Warn"}, {13,"",""}, {14,"FLOW_ERR_D","Alarm"}, {15,"",""},
    //     {16,"",""}, {17,"WASTE_FULL","Alarm"},
    //     {18,"DOOR_OPEN","Alarm"}, {19,"CPU_OVERHEAT","Warn"}, {20,"MEM_HIGH","Warn"},
    //     {21,"MQTT_OFFLINE","Alarm"}, {22,"PAY_FAIL_GATEWAY","Warn"}, {23,"TX_ABORTED","Alarm"},
    //     {24,"SELFCHK_FAIL","Alarm"}, {25,"OTA_DL_FAIL","Warn"}, {26,"OTA_VFY_FAIL","Alarm"},
    //     {27,"LEVEL_SNR_FAIL","Warn"}, {28,"LEVEL_SNR_FAIL","Alarm"},{29,"CASH_MOD_ERR","Alarm"}, {30,"TOUCH_OFFLINE","Alarm"},
    //     {31,"MAINTENANCE_MODE","Alarm"}
    //     // 31~45 依需要補上 RESERVED 等 46 47 已定義
    //     // Drift
    // };

    inline static const QVector<AlarmDef> kAlarms = {
        {  0, "MQTT_OFFLINE",   "Alarm"},   // MQTT 斷線
        {  1, "SERVICE_STOPPED","Alarm"},   // 服務停止
        {  2, "CPU_OVERHEAT",   "Alarm"},   // CPU 過溫
        {  3, "MEM_HIGH",       "Warn"},    // 記憶體高佔用
        {  4, "CFG_REJECTED",   "Warn"},    // 登入驗證失敗
        {  5, "LOGIN_WARN",     "Warn"},    // 登入異常預警
        {  6, "PHISHING_SUS",   "Warn"},    // 可疑釣魚行為
        {  7, "MOTOR_STALLED",  "Warn"},    // 馬達堵轉
        {  8, "LASER_RX_FAULT", "Alarm"},   // 雷射接收故障
        {  9, "METAL_FAIL",     "Alarm"},   // 金屬感測故障
        { 10, "LEN_LASER_FAIL", "Alarm"},   // 長度雷射故障
        { 11, "HT_LASER_FAIL",  "Alarm"},   // 高度雷射故障
        { 12, "TOUCH_OFFLINE",  "Alarm"},   // 觸控離線
        { 13, "TOUCH_ABUSE",    "Warn"},    // 觸控濫用
        { 14, "",    "N/A"},
        { 15, "",    "N/A"},
        { 16, "BELT_JAM",       "Alarm"},   // 馬達異常或輸送帶卡住
        { 17, "LASER_RECEIVE_FAULT",   "Alarm"}, //入口雷射接收異常
        { 18, "LEN_LASER_RECEIVE_FAIL","Alarm"}, //入口長度接收異常
        { 19, "HT_LASER_RECEIVE_FAIL", "Alarm"}, //入口高度接收異常
        { 20, "",    "N/A"},
        { 21, "",    "N/A"},
        { 22, "",    "N/A"},
        { 23, "",    "N/A"},
        { 24, "BIN_WARN",       "Warn"},    // 回收桶達警戒
        { 25, "MACHINE_SLEEP",  "Alarm"},   // 機台休眠
        { 26, "BIN_FULL",       "Alarm"},   // 回收桶已滿
        { 27, "",    "N/A"},
        { 28, "",    "N/A"},
        { 29, "",    "N/A"},
        { 30, "",    "N/A"},
        { 31, "",    "N/A"},
        { 32, "",    "N/A"},
        { 33, "",    "N/A"},
        { 34, "",    "N/A"},
        { 35, "",    "N/A"},
        { 36, "",    "N/A"},
        { 37, "",    "N/A"},
        { 38, "",    "N/A"},
        { 39, "",    "N/A"},
        { 40, "",    "N/A"},
        { 41, "",    "N/A"},
        { 42, "",    "N/A"},
        { 43, "",    "N/A"},     // 你原始表裡 43/44 都標 RES_39 → 我修正
        { 44, "NET_DISCONNECT", "Alarm"},     // 網路斷線
        { 45, "ENGINEERING",    "Alarm"},     // 工程模式
        { 46, "SYS_SHUTDOWN",   "Info"},    // 關機
        { 47, "SYS_UP",         "Info"}     // 開機
    };

    QString computeCode48();                 // 依目前感測/狀態算出 48bit
    QString summarizeMessage(const QString& code); // 從 code 挑一個代表性的英文 message (≤30bytes)
    QString summarizeLevel(const QString& code);   // 任一 Alarm 則 "Alarm"，否則有 Warn 則 "Warn"，否則 "Info"
    // void sendTelemetry(const QString& code);       // 組 payload 並 publish
    QString decideStatus(const QString& code);
    void onMqttConnected();
    // void trigetTelemetry(QString &status,QString &level,QString &code,QString &message,QString &count,QString &data);

    bool hasPublishedOnStartup = false;

    QDateTime startTime ;
    QDateTime endTime ;
    ErrorItem_Dialog *test_err_dlg = nullptr;
    int alertCount = 0;

    bool bit0LeftMost = false; // false = bit0 在最右邊 (LSB)
    inline int idxForBit(int b) const { return bit0LeftMost ? b : (47 - b); }

    //自動恢復雷射檢測用
    int m_failCount = 0;//異常後馬達自檢次數
    QTimer recoveryTimer;//每3分鐘異常恢復自檢三次用
    QTimer recoveryStopTimer;//12秒後關閉
    bool m_isLocked = false; //判斷三次後狀態鎖定旗標
    bool m_isRecovering = false;//判斷執行中的旗標
    int m_sensorCheckCount = 0;

    QTimer statusUpdateTimer; //每10分鐘再次發送機台狀態

};
