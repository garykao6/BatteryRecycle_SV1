#include "global.h"
namespace global {

    //輪播取得物件指標
    static PlayTool *_videoObject = nullptr;
    void setVideoShow(PlayTool *p)
    {
        _videoObject = p;
    }

    PlayTool *getVideoShow()
    {
        return _videoObject;
    }

    //廣告輪播取得物件指標
    static PlayTool *_AdObject = nullptr;
    void setAdShow(PlayTool *p)
    {
        _AdObject = p;
    }
    PlayTool *getAdShow()
    {
        return _AdObject;
    }

    PlayTool& videoShow() {
        static PlayTool instance;  // 第一次呼叫時才建構
        return instance;
    }

    //碳量計算
    double& carbonReduction() {
        static double value = 0.0; // 初始化為0
        return value;
    }

    // mqtt
    MqttHelper& mqtt() {
        static MqttHelper instance;
        return instance;
    }

    int& treeCount() {
        static int value = 0; // 初始化為0
        return value;
    }

    QString deviceId = ""; //機台序號
    QString phonenumber;   // 這裡真正分配記憶體
    QString username;   // 這裡真正分配記憶體
    QString points; //點數
    QString loginTime = "";//登入時間

    //機台碳足跡
    double total_carbonReduction = 0;

    // 軟韌體版本
    // QString SWVersion = "SmartBatteryV2-1.1.0";
    QString SWVersion = "SmartBatteryV1-1.1.8";

    //站點名稱
    QString storeInfo = "";

    //工程模式操作人員id
    QString  op_id = "000000";

    //anydeskId
    QString anydeskId = "";

    //馬達調用DeviceAPI_Run時狀態
    int motorstatus = 0;

    //休眠時間
    bool sleepEnable = false;
    QString sleepStrart="";
    QString sleepEnd="";

    // double estWeightG = 0.0; //總重量 公斤 工程模式清零
    double& estWeightG() {
        static double v = 0.0;  // 全程唯一、延遲初始化
        return v;
    }

    //電池重量係數
    const double batteryRate_1 = 95.1;
    const double batteryRate_2 = 48.9;
    const double batteryRate_3 = 17.4;
    const double batteryRate_4 = 8.66;
    const double batteryRate_5 = 9.2;
    const double batteryRate_6 = 6.5;
    const double batteryRate_9 = 39.8;

    //馬達堵轉
    bool MotorStalled = false;

    void reset()
    {
        QString phonenumber="";   // 這裡真正分配記憶體
        QString username="";   // 這裡真正分配記憶體
        QString points=""; //點數
        QString loginTime = "";
    }

    // DeviceMonitor *monitor = nullptr;
    //機台狀態偵測及心跳
    QPointer<DeviceMonitor>& monitor() {
        static QPointer<DeviceMonitor> ptr;  // 初始為 nullptr；目標被刪會自動變回 nullptr
        return ptr;
    }

    QPointer<IdleWatcher>& idlewatcher()
    {
        static QPointer<IdleWatcher> ptr;  // 初始為 nullptr；目標被刪會自動變回 nullptr
        return ptr;
    }

    //建立聲音播放器
    QMediaPlayer& soundPlayer() {
        static QMediaPlayer player;
        static QAudioOutput output;   // 也放 static，確保生命週期夠長
        player.setAudioOutput(&output);
        return player;
    }

    //聲音播放
    void playSound(const QString& path) {
        QMediaPlayer& p = soundPlayer();
        p.stop();
        // p.setSource(QUrl::fromLocalFile(path));
        p.setSource(QUrl::fromLocalFile(global::resolveSound(path)));
        p.play();
    }

    QString resolveSound(const QString& name) {
        // return QDir(QCoreApplication::applicationDirPath()).filePath(QString("sounds/%1").arg(name));
        return QDir(QCoreApplication::applicationDirPath()).filePath(name);
    }
    //聲音暫停
    void playStop()
    {
        QMediaPlayer& p = soundPlayer();
        p.stop();
    }

    // ===== 共用 config 路徑 =====
    static QString configPath() {
        //系統路徑內
        // const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        // QDir().mkpath(dir); // 確保資料夾存在
        // return QDir(dir).filePath("MachineConfig.json");

        const QString dir = QCoreApplication::applicationDirPath();
        return QDir(dir).filePath("MachineConfig.json");
    }

    void writeConfig(const QString &key, const QJsonValue &value)//寫入config檔
    {
        // // 存放位置：~/.config/你的APP/config.json
        // const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        // QDir().mkpath(dir);
        // const QString path = dir + "/MachineConfig.json";
        const QString path = configPath();

        QJsonObject obj;

        // 如果檔案存在，先讀取
        QFile f(path);
        if (f.exists() && f.open(QIODevice::ReadOnly)) {
            const auto doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject())
                obj = doc.object();
            f.close();
        }

        // 更新 key
        obj[key] = value;

        // 寫回檔案
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QJsonDocument doc(obj);
            f.write(doc.toJson(QJsonDocument::Indented)); // 美化縮排
            f.close();
            qDebug() << "✅ writeConfig 寫入成功:" << key << "=" << value;
        } else {
            qWarning() << "⚠️ writeConfig 寫入失敗:" << f.errorString();
        }
    }

    QJsonObject readConfig() //讀取整個config檔
    {
        // const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        // const QString path = QDir(dir).filePath("MachineConfig.json");

        const QString path = configPath();

        QFile f(path);
        if (!f.exists()) {
            qWarning() << "⚠️ config.json 不存在，回傳空物件";
            return {};
        }
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "⚠️ 無法開啟 config.json" << f.errorString();
            return {};
        }

        const QByteArray data = f.readAll();
        f.close();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "⚠️ JSON 解析錯誤:" << err.errorString();
            return {};
        }

        return doc.object();
    }

    // QString readConfigValue(const QString& key, const QString& def) //讀取config檔 單一欄位
    // {
    //     QJsonObject obj = readConfig();
    //     return obj.value(key).toString(def); // 取 key，若不存在或不是字串 → 回傳 def
    // }

    QVariant readConfigValue(const QString &key, const QVariant &def)
    {
        QJsonObject obj = readConfig();   // ← 你現成的 readConfig()，回傳 QJsonObject
        QJsonValue v = obj.value(key);

        if (v.isUndefined() || v.isNull()) return def;

        if (v.isBool())   return v.toBool();
        if (v.isDouble()) return v.toDouble();
        if (v.isString()) return v.toString();
        if (v.isArray())  return v.toArray().toVariantList();
        if (v.isObject()) return v.toObject().toVariantMap();

        return def;
    }


    void initConfig() //初始建立檔案
    {
        // // 1. 找 config 路徑
        // const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        // QDir().mkpath(dir);  // 確保資料夾存在
        // const QString path = QDir(dir).filePath("MachineConfig.json");

        const QString path = configPath();

        // 2. 檢查是否已存在
        if (QFile::exists(path)) {
            qDebug() << "✅ config.json 已存在:" << path;
            return;
        }

        // 3. 建立預設內容
        QJsonObject obj;
        obj["deviceId"] = "FLCBRM03_N2508001";//機台號碼
        obj["totalWeight"]  = 0;//電池總重量
        obj["normalShutdown"] = true;// ⚑ 預設：正常關閉（第一次執行視為乾淨狀態）
        // obj["sw"] = "smartbatteryV2-1.1.0";
        obj["total_carbonReduction"] = 0;
        obj["powerOffTime"]="";
        obj["keepStop"] = false;
        obj["sleepEnable"] = false; //休眠狀態
        obj["sleepStrart"] = "00:00";
        obj["sleepEnd"] = "00:00";

        // 4. 存檔
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            qWarning() << "⚠️ 無法建立 config.json" << f.errorString();
            return;
        }
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        f.close();

        qDebug() << "📂 已建立預設 config.json:" << path;
    }


    //機台狀態
    bool isMaintenanceMode = false; //工程模式
    bool isStopMode = false; //暫停模式
    bool isPhishing = false; //釣魚
    bool isLoginfalse = false;//登入失敗

}
