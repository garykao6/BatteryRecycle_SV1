#include "backstage.h"
#include "ui_backstage.h"

//偵測銀幕亮度bus
inline int detectBusNumber() {
    QProcess p;
    p.start("ddcutil", {"detect"});
    p.waitForFinished(1500);
    const QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    // 範例片段: "I2C bus: /dev/i2c-3"
    QRegularExpression rx(R"(I2C\s+bus:\s+/dev/i2c-(\d+))");
    auto m = rx.match(out);
    return m.hasMatch() ? m.captured(1).toInt() : -1;
}

backstage::backstage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::backstage)
{
    ui->setupUi(this);
    timer = new QTimer(this);
    timer->setInterval(100);

    constexpr double CARBON_COEFF = 3.684;

    ui->voice_control->setRange(0, 100);
    ui->voice_control->setValue(80);

    // 預設音量
    QProcess::execute("amixer", {"set", "Master",  "80%", "unmute"});
    // 控制音量
    connect(ui->voice_control, &QSlider::valueChanged, this, [=](int value){
        // value = 0 ~ 100
        QString vol = QString::number(value) + "%";
        QProcess::execute("amixer", {"set", "Master", vol, "unmute"});
    });

    ui->brightness_control->setRange(0, 100);
    ui->brightness_control->setValue(80);

    // 預設銀幕亮度
    // startDetached
    int bus = detectBusNumber(); //bus偵測
    QProcess::startDetached("ddcutil",
                            {"--bus=" + QString::number(bus), "setvcp", "10", QString::number(80)});
    // 控制銀幕亮度
    connect(ui->brightness_control, &QSlider::valueChanged, this, [=](int value){
        // percent = std::clamp(percent, 0, 100);
        QProcess::startDetached("ddcutil",
                                {"--bus=" + QString::number(bus), "setvcp", "10", QString::number(value)});
    });

    //接收電池
    connect(timer, &QTimer::timeout, this, [this]() {

        // Gate laser
        static int lastGate = -1;
        int curGate = DeviceAPI_GetSendor(0);
        if (curGate != lastGate) {
            lastGate = curGate;
            QString color = (curGate == 0) ? "#ffa500" : "#00ff00"; // 1綠，0橘
            ui->gate_laser_lb->setStyleSheet("background-color:" + color + ";");
            // emit idleWatchPoke();//避免進入待機
        }

        // Metal laser
        static int lastMetal = -1;
        int curMetal = DeviceAPI_GetSendor(3);
        if (curMetal != lastMetal) {
            lastMetal = curMetal;
            QString color = (curMetal == 0) ? "#ffa500" : "#00ff00";
            qDebug() << "✅ color = " << color;
            ui->Metal_laser_lb->setStyleSheet("background-color:" + color + ";");
            // emit idleWatchPoke();
        }

        // Length laser
        static int lastLength = -1;
        int curLength = DeviceAPI_GetSendor(1);
        if (curLength != lastLength) {
            lastLength = curLength;
            QString color = (curLength == 0) ? "#ffa500" : "#00ff00";
            ui->length_laser_lb->setStyleSheet("background-color:" + color + ";");
            // emit idleWatchPoke();
        }

        // Height laser
        static int lastHeight = -1;
        int curHeight = DeviceAPI_GetSendor(2);
        if (curHeight != lastHeight) {
            lastHeight = curHeight;
            QString color = (curHeight == 0) ? "#ffa500" : "#00ff00";
            ui->height_laser_lb->setStyleSheet("background-color:" + color + ";");
            // emit idleWatchPoke();
        }

        // 這裡也加電池檢查
        // 1號電池 (index=2)
        static int lastB1 = 0;
        int curB1 = DeviceAPI_GetBattery(2);
        if (curB1 != lastB1) {
            lastB1 = curB1;
            ui->current_battery_lb->setText("1號電池");
            global::estWeightG()+= global::batteryRate_1;
            global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double
            global::total_carbonReduction += global::batteryRate_1 * CARBON_COEFF; //機台碳足跡
            global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
            ui->totalweight_lb->setText(QString::number(global::estWeightG(), 'f', 2));
            // emit idleWatchPoke();
        }

        // 3號電池 (index=1)
        static int lastB3 = 0;
        int curB3 = DeviceAPI_GetBattery(1);
        if (curB3 != lastB3) {
            lastB3 = curB3;
            ui->current_battery_lb->setText("3號電池");
            global::estWeightG()+= global::batteryRate_3;
            global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double
            global::total_carbonReduction += global::batteryRate_3 * CARBON_COEFF; //機台碳足跡
            global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
            ui->totalweight_lb->setText(QString::number(global::estWeightG(), 'f', 2));
            // emit idleWatchPoke();
        }

        // 4號電池 (index=3)
        static int lastB4 = 0;
        int curB4 = DeviceAPI_GetBattery(3);
        if (curB4 != lastB4) {
            lastB4 = curB4;
            ui->current_battery_lb->setText("4號電池");
            global::estWeightG()+= global::batteryRate_4;
            global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double
            global::total_carbonReduction += global::batteryRate_4 * CARBON_COEFF; //機台碳足跡
            global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
            ui->totalweight_lb->setText(QString::number(global::estWeightG(), 'f', 2));
            // emit idleWatchPoke();
        }

        // 5號電池 (index=4)
        static int lastB5 = 0;
        int curB5 = DeviceAPI_GetBattery(4);
        if (curB5 != lastB5) {
            lastB5 = curB5;
            ui->current_battery_lb->setText("5號電池");
            global::estWeightG()+= global::batteryRate_5;
            global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double
            global::total_carbonReduction += global::batteryRate_5 * CARBON_COEFF; //機台碳足跡
            global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
            ui->totalweight_lb->setText(QString::number(global::estWeightG(), 'f', 2));
            // emit idleWatchPoke();
        }

        // 6號電池 (index=5)
        static int lastB6 = 0;
        int curB6 = DeviceAPI_GetBattery(5);
        if (curB6 != lastB6) {
            lastB6 = curB6;
            ui->current_battery_lb->setText("6號電池");
            global::estWeightG()+= global::batteryRate_6;
            global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double
            global::total_carbonReduction += global::batteryRate_6 * CARBON_COEFF; //機台碳足跡
            global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
            ui->totalweight_lb->setText(QString::number(global::estWeightG(), 'f', 2));
            // emit idleWatchPoke();

        }

        // 2&9號電池 (index=6)
        static int lastB29 = 0;
        int curB29 = DeviceAPI_GetBattery(6);
        if (curB29 != lastB29) {
            lastB29 = curB29;
            ui->current_battery_lb->setText("2號或9號電池");
            global::estWeightG()+= global::batteryRate_2;
            global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double
            global::total_carbonReduction += global::batteryRate_2 * CARBON_COEFF; //機台碳足跡
            global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
            ui->totalweight_lb->setText(QString::number(global::estWeightG(), 'f', 2));
            // emit idleWatchPoke();
        }

        // 異物 (index=0)
        static int lastobstacle = 0;
        int obstacle = DeviceAPI_GetBattery(0);
        if (obstacle != lastobstacle) {
            lastobstacle = obstacle;
            ui->current_battery_lb->setText("異物");
            // emit idleWatchPoke();
        }

        // 釣魚 (index=7)
        static int last_err_op_count = 0;
        int err_op_count = DeviceAPI_GetBattery(7);
        if (err_op_count != last_err_op_count) {
            last_err_op_count = err_op_count;
            ui->current_battery_lb->setText("釣魚");
            // emit idleWatchPoke();
        }

        changeStatusLight();//當前狀態

        emit idleWatchPoke();
    });

}

backstage::~backstage()
{
    delete ui;
}

void backstage::showEvent(QShowEvent *event)
{
    function_Hidden();//依登錄權限關閉功能
    global::idlewatcher()->pause();//暫停待機
    // ui->totalweight_lb->setText(global::readConfigValue("totalWeight","0000").toString());
    double totalWeight = global::readConfigValue("totalWeight", "0").toDouble();
    ui->totalweight_lb->setText(QString::number(totalWeight, 'f', 2));
    ui->deviceId_lb->setText(global::readConfigValue("deviceId","0000").toString());
    // ui->sw_fw_version_lb->setText(global::readConfigValue("sw/fw","0000").toString());

    //軟體版本顯示
    ui->sw_fw_version_lb->setText(global::SWVersion);
    ui->sw_fw_version_lb->adjustSize();

    //關機時間顯示
    showSystemOffTIme();

    global::isMaintenanceMode = true;
    global::isStopMode = false;//不在暫停模式
    global::writeConfig("keepStop",global::isStopMode);//寫入檔案  //斷電維持暫停服務

    // changeStatusLight();//當前狀態
    // DeviceAPI_Open();
    // m_motorstatus = DeviceAPI_Run(); //開始收電池
    global::motorstatus = DeviceAPI_Run(); //開始收電池
    global::mqtt().publishMaintenanceCommand(global::op_id, "LOGIN", "", "", "");//登入訊息
    global::monitor()->checkStatus();

    timer->start();

    //燈號先設為正常狀態
    ui->gate_laser_lb->setStyleSheet("image: none; background-color: #00ff00;");
    ui->length_laser_lb->setStyleSheet("image: none; background-color: #00ff00;");
    ui->height_laser_lb->setStyleSheet("image: none; background-color: #00ff00;");
    ui->Metal_laser_lb->setStyleSheet("image: none; background-color: #00ff00;");
    //馬達燈號先設為正常狀態
    ui->motor_lb->setStyleSheet("image: none; background-color: #00ff00;");
}

void backstage::hideEvent(QHideEvent *event)
{
    // DeviceAPI_Close();//關閉
    DeviceAPI_Run_Stop(); //關閉收電池
    timer->stop();
    // global::isMaintenanceMode = false;
}

//關閉功能及隱藏
void backstage::function_Hidden()
{
    QMap<QString, int> permissionMap;
    permissionMap["ecoco"] = 1;//營運清運
    permissionMap["ecoco1"] = 3;//研發權限
    permissionMap["ecoco2"] = 2;//營運維修

    int userLevel = permissionMap.value(global::op_id, 1); // 找不到就給 0 (Guest)

    //要權限管理的物件放這邊
    QList<QWidget*> adminWidgets = {
        ui->setting_btn,
        ui->clear_carbon_btn,
    };

    for (QWidget* widget : adminWidgets) {
        widget->setVisible(userLevel > 1);
        widget->setEnabled(userLevel > 1);
    }

}

void backstage::on_home_btn_clicked()
{
    global::isMaintenanceMode = false;
    if (global::idlewatcher()) global::idlewatcher()->resume();//待機恢復監控
    global::mqtt().publishMaintenanceCommand(global::op_id, "LOGOUT", "", "", "");//登出訊息
    QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
    if (stack) {
        stack->setCurrentIndex(0);   // 切至首頁
        global::getVideoShow()->start();// 播放並顯示影片輪播
        global::getVideoShow()->show();
        // global::isMaintenanceMode = false;
        global::monitor()->checkStatus();
    }
}


void backstage::on_stop_service_btn_clicked()
{
    qDebug() << "✅ 觸發隱藏功能，意切換頁面";
    global::isStopMode = true;
    global::writeConfig("keepStop",global::isStopMode);//寫入檔案  //斷電維持暫停服務
    if (global::idlewatcher()) global::idlewatcher()->pause();//待機暫停監控
    //LedAPI_Set(0x00ff00);//red 顯示紅燈
    LedAPI_Select(1);

    QStackedWidget *stack = qobject_cast<QStackedWidget*>(this->parentWidget());
    QWidget* err_dlg = stack->widget(7); // 異常頁面
    if (err_dlg) stack->setCurrentWidget(err_dlg);
    global::getVideoShow()->stop(); // 暫停
    global::getVideoShow()->hide(); //隱藏影片輪播
    // global::isStopMode = true;
    global::monitor()->checkStatus(); //調用檢查函式
    global::mqtt().publishMaintenanceCommand(global::op_id, "STOP", "", "", "");
}


void backstage::on_system_down_btn_clicked()
{
    qDebug() << "👋 程式即將正常退出";
    //LedAPI_Close();
    DeviceAPI_Close();
    global::mqtt().publishMaintenanceCommand(global::op_id, "SYS_SHUTDOWN", "", "", "");
    global::monitor()->sendMessage("down","Alarm","","關機","",""); //調用檢查函式

    // 執行系統關機
    QCoreApplication::quit();
    QProcess::startDetached("sudo", {"poweroff"});

}


void backstage::on_system_reboot_btn_clicked()
{
    qDebug() << "👋 程式即將正常退出";
    //LedAPI_Close();
    DeviceAPI_Close();
    global::mqtt().publishMaintenanceCommand(global::op_id, "SYS_REBOOT", "", "","");
    global::monitor()->sendMessage("down","Alarm","","重新啟動","",""); //調用檢查函式

    // 執行系統重啟
    QCoreApplication::quit();
    QProcess::startDetached("sudo", {"reboot"});
}


void backstage::on_Forward_btn_clicked()
{
    // DeviceAPI_Open();
    DeviceAPI_MotorForward();//正轉
    global::mqtt().publishMaintenanceCommand(global::op_id, "BELT_FORWARD", "", "", "");
}


void backstage::on_Reverse_btn_clicked()
{
    // DeviceAPI_Open();
    DeviceAPI_MotorBack(); //反轉
    global::mqtt().publishMaintenanceCommand(global::op_id, "BELT_REVERSE", "", "", "");
}


void backstage::on_stop_btn_clicked()
{
    DeviceAPI_MotorStop();  //馬達停止
    // DeviceAPI_Close();
    global::mqtt().publishMaintenanceCommand(global::op_id, "BELT_STOP", "", "", "");
}


void backstage::on_off_btn_clicked()
{
    //LedAPI_Set(0x000000);//關燈
    LedAPI_Select(0);
    global::mqtt().publishMaintenanceCommand(global::op_id, "RING_TEST_STOP", "", "", "");
}


void backstage::on_red_btn_clicked()
{
    // LedAPI_Run();
    //LedAPI_Set(0x00ff00);//red
    LedAPI_Select(1);
    global::mqtt().publishMaintenanceCommand(global::op_id, "RING_RED", "", "", "");
}


void backstage::on_blue_btn_clicked()
{
    // LedAPI_Run();
    //LedAPI_Set(0x0000ff);//blue
    LedAPI_Select(3);
    global::mqtt().publishMaintenanceCommand(global::op_id, "RING_BLUE", "", "", "");
}


void backstage::on_green_btn_clicked()
{
    // LedAPI_Run();
    //LedAPI_Set(0xff0000);//green
    LedAPI_Select(2);
    global::mqtt().publishMaintenanceCommand(global::op_id, "RING_GREEN", "", "", "");
}


void backstage::on_clear_btn_clicked()
{
    global::estWeightG() = 0;
    ui->totalweight_lb->setText(QString::number(global::estWeightG()));
    global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double
    global::monitor()->checkStatus(); //調用檢查函式
    global::mqtt().publishMaintenanceCommand(global::op_id, "BIN_RESET", "", QString::number(global::estWeightG()), "");
}


void backstage::on_setting_btn_clicked()
{
    // setting_Dialog set_dlg(this);
    // set_dlg.exec();
    auto *set_dlg = new setting_Dialog();
    int ret = set_dlg->exec();   // ✅ 只呼叫一次 exec()，保存結果
    // set_dlg->show();
    if (ret == QDialog::Accepted) {
        ui->deviceId_lb->setText(global::readConfigValue("deviceId","0000").toString());
        showSystemOffTIme();//關機時間讀取
        // 執行系統重啟
        QCoreApplication::quit();
        QProcess::startDetached("sudo", {"reboot"});
        // global::writeConfig("deviceId", global::total_carbonReduction); //機台碳足跡寫回JSON
    }
    if (ret == setting_Dialog::SET_POWEROFF) {
        showSystemOffTIme();//關機時間讀取
    }

    delete set_dlg;   // ✔️ exec() 結束後記得刪除

    // // 執行系統重啟
    // QCoreApplication::quit();
    // QProcess::startDetached("sudo", {"reboot"});
}

void backstage::changeStatusLight()

{

    //net 網路先因為檢測斷線會把mqtt也斷線
    // if(global::monitor()->lastCode48[idxForBit(44)]=="1")
    if(global::monitor()->checkNetwork())
        ui->net_lb->setStyleSheet("image: none; background-color: #ff0000;");
    else
        ui->net_lb->setStyleSheet("image: none; background-color: #00ff00;");

    //mqtt
    // if(global::monitor()->lastCode48[idxForBit(0)]=="1")
    if(global::mqtt().m_client->state() != QMqttClient::Connected)
        ui->mqtt_lb->setStyleSheet("image: none; background-color: #ff0000;");
    else
        ui->mqtt_lb->setStyleSheet("image: none; background-color: #00ff00;");

    //cpu
    // if(global::monitor()->lastCode48[idxForBit(2)]=="1")
    if(global::monitor()->getCpuTemperature()>=80)
        ui->cpu_lb->setStyleSheet("image: none; background-color: #ff0000;");
    else
        ui->cpu_lb->setStyleSheet("image: none; background-color: #00ff00;");

    //記憶體
    // if(global::monitor()->lastCode48[idxForBit(2)]=="1")
    if(global::monitor()->getMemoryUsage()>=85)
        ui->mem_lb->setStyleSheet("image: none; background-color: #ff0000;");
    else
        ui->mem_lb->setStyleSheet("image: none; background-color: #00ff00;");


    //檢測函式 global::motorstatus = int DeviceAPI_Run(void) 回傳之結果
    qDebug()<<"電池機Errcode "<<global::motorstatus;
    switch (global::motorstatus) {
    case 1:
        //馬達異常
        ui->motor_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    case 2:
        //馬達異常
        ui->motor_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    case 4:
        //入口雷射接收異常
        ui->gate_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    case 5:
        //長度雷射接收異常
        ui->length_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    case 6:
        //高度雷射接收異常
        ui->height_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    default:
        break;
    }

    //檢測函式 int  DeviceAPI_GetMotorErrorCode(void) 回傳之結果
    switch (DeviceAPI_GetMotorErrorCode()) {
    case 7:
        ui->gate_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    case 8:
        ui->length_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    case 9:
        ui->height_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    case 10:
        ui->Metal_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        ui->motor_lb->setStyleSheet("image: none; background-color: #ff0000;");
        break;
    default:
        break;
    }

//     //motor
//     // if(global::monitor()->lastCode48[idxForBit(16)]=="1")
//     if(global::motorstatus!=0 || DeviceAPI_GetMotorErrorCode() != 0)
//     {
//         //馬達或雷射有一個錯就進正個判斷
//         // ui->motor_lb->setStyleSheet("image: none; background-color: #ff0000;");
//         //0 11 12 13 14 15 16是馬達錯誤  7 8 9 10 是雷射錯誤
//         switch (DeviceAPI_GetMotorErrorCode()) {
//         case 0:
//             ui->motor_lb->setStyleSheet("image: none; background-color: #ff0000;");
//             break;
//         case 7:
//             ui->gate_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
//             break;
//         case 8:
//             ui->length_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
//             break;
//         case 9:
//             ui->height_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
//             break;
//         case 10:
//             ui->Metal_laser_lb->setStyleSheet("image: none; background-color: #ff0000;");
//             break;
//         case 11:
//         case 12:
//         case 13:
//         case 14:
//         case 15:
//         case 16:
//             ui->motor_lb->setStyleSheet("image: none; background-color: #ff0000;");
//             break;
//         default:
//             break;
//         }
//     }
//     else
//     {
//         //qDebug()<<"DeviceAPI_GetMotorErrorCode 正常";
//         //馬達
//         ui->motor_lb->setStyleSheet("image: none; background-color: #00ff00;");
//         //雷射
// //         ui->gate_laser_lb->setStyleSheet("image: none; background-color: #00ff00;");
// //         ui->length_laser_lb->setStyleSheet("image: none; background-color: #00ff00;");
// //         ui->height_laser_lb->setStyleSheet("image: none; background-color: #00ff00;");
// //         ui->Metal_laser_lb->setStyleSheet("image: none; background-color: #00ff00;");
//     }


    //battery_full
    // if(global::monitor()->lastCode48[idxForBit(24)]=="1")
    if(global::readConfigValue("totalWeight", "0").toDouble()>=32000)
        ui->full_lb->setStyleSheet("image: none; background-color: #ff0000;");
    else
        ui->full_lb->setStyleSheet("image: none; background-color: #00ff00;");



}

void backstage::showSystemOffTIme()
{
    QString powerOffTime = global::readConfigValue("powerOffTime","").toString();
    if(powerOffTime.contains(":"))
    {
        ui->powerOffTime_lb->setText(powerOffTime);
    }else
    {
        ui->powerOffTime_lb->setText("null");
    }
    ui->powerOffTime_lb->adjustSize();
}

void backstage::on_clear_carbon_btn_clicked()
{
    // global::total_carbonReduction = global::readConfigValue("total_carbonReduction","0").toDouble();
    global::total_carbonReduction = 0;
    global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
}

