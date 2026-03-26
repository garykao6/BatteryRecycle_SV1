#ifndef GLOBAL_H
#define GLOBAL_H
#include "playtool.h"
#include "mqtthelper.h"
#include <QAudioOutput>
#include <QCoreApplication>
#include "devicemonitor.h"
#include <QPointer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "idlewatcher.h"

namespace global {
    // extern PlayTool videoShow;  // 只宣告，不定義

    void setVideoShow(PlayTool *p);
    PlayTool *getVideoShow();

    void setAdShow(PlayTool *p);
    PlayTool *getAdShow();

    PlayTool& videoShow();


    // 減碳量（公斤）
    double& carbonReduction();

    // 全域 MQTT 物件
    MqttHelper& mqtt();

    // 樹的數量
    int& treeCount();

    //機台序號
    extern QString deviceId;

    // 儲存會員電話
    extern QString phonenumber;

    // 使用者名稱
    extern QString username;
    extern QString points;

    //登入時間
    extern QString loginTime;

    //重量統計
    double& estWeightG();   // 回傳「同一個」實體的參考

    //工程模式操作人員id
    extern QString  op_id;

    //機台碳排廢
    extern double total_carbonReduction;

    //軟韌體版本
    extern QString SWVersion;

    //站點名稱
    extern QString storeInfo;

    //anydeskId
    extern QString anydeskId;

    //馬達調用DeviceAPI_Run時狀態
    extern int motorstatus;

    //休眠時間
    extern bool sleepEnable;
    extern QString sleepStrart;
    extern QString sleepEnd;


    //電池重量係數
    extern const double batteryRate_1;
    extern const double batteryRate_2;
    extern const double batteryRate_3;
    extern const double batteryRate_4;
    extern const double batteryRate_5;
    extern const double batteryRate_6;
    extern const double batteryRate_9;

    //馬達堵轉狀態
    extern bool MotorStalled;


    void reset();//重置

    // extern DeviceMonitor *monitor;
    //機台狀態偵測
    QPointer<DeviceMonitor>& monitor();

    //待機檢測
    QPointer<IdleWatcher>& idlewatcher();

    // 單例播放器
    QMediaPlayer& soundPlayer();

    // 播放音效
    void playSound(const QString& path);
    // 音效暫停
    void playStop();



    QString resolveSound(const QString& name); //路徑解碼用 在playSound內使用
    void writeConfig(const QString &key, const QJsonValue &value);//寫入config檔
    QJsonObject readConfig(); //讀取整個config檔
    // QString readConfigValue(const QString& key, const QString& def = QString()); //讀取單一值
    QVariant readConfigValue(const QString &key, const QVariant &def = {});
    void initConfig();//沒有config.json建立遇上檔案

    //機台狀態
    extern bool isMaintenanceMode;//工程模式
    extern bool isStopMode;//暫停
    extern bool isPhishing; //釣魚
    extern bool isLoginfalse;//登入失敗
}

#endif // GLOBAL_H
