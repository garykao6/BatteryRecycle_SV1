#include "nmb_putinbatterypage.h"
#include "ui_nmb_putinbatterypage.h"
#include <QStackedWidget>
#include "clockbus.h"
#include "DeviceAPI.h"
#include "global.h" //音效
#include "machinerecord_dialog.h"//記得刪除
#include "machineerr_dialog.h"//機台異常

nmb_PutInBatteryPage::nmb_PutInBatteryPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::nmb_PutInBatteryPage)
    ,gpio(5) // 使用 GPIO5
{
    ui->setupUi(this);
    ui->showtxt_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    countDownTimer = new QTimer(this);


    video_player = new QMediaPlayer(ui->Top_Widget);
    video_output = new QVideoWidget(ui->Top_Widget);
    video_player->setVideoOutput(video_output);
    QString appDir   = QCoreApplication::applicationDirPath();
    QString videoAbs = QDir(appDir).filePath("mp4/教學頁.mp4"); // 相對於執行檔
    QUrl url = QUrl::fromLocalFile(videoAbs);
    video_player->setSource(url);
    video_player->setLoops(QMediaPlayer::Infinite);  // 連播
    video_player->pause();


    err_dlg = new ErrorItem_Dialog(nullptr); // ⬅ 不要給 parent，避免被限制
    err_dlg->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    err_dlg->showIdentify();

    connect(ClockBus::instance(),&ClockBus::minuteTick,this,[this](const QDateTime&, const QString& s) //使用全域變數QTimer每分鐘更新時間
            {
                ui->time_lb->setText(s);
            });

    // ======= 新增：每0.1秒更新馬達/電池/感測器 =======
    timer1 = new QTimer(this);
    connect(timer1, &QTimer::timeout, this, [this]() {

        motorStatus = DeviceAPI_GetMotorStatus();//馬達狀態
        battery3     = DeviceAPI_GetBattery(1);//3號電池
        battery1     = DeviceAPI_GetBattery(2);//1號電池
        battery4     = DeviceAPI_GetBattery(3);//4號電池
        battery5     = DeviceAPI_GetBattery(4);//5號電池
        battery6     = DeviceAPI_GetBattery(5);//6號電池
        battery29    = DeviceAPI_GetBattery(6);//2&9號電池
        err_op_count = DeviceAPI_GetBattery(7);//釣魚
        obstacle     = DeviceAPI_GetBattery(0);//異物

        sensor0     = DeviceAPI_GetSendor(0);//位置感測
        sensor1     = DeviceAPI_GetSendor(1);//長度感測
        sensor2     = DeviceAPI_GetSendor(2);//高度感測
        sensor3     = DeviceAPI_GetSendor(3);//金屬感測
        // 如果有任一個值改變就印出
        // if (motorStatus   != lastMotorStatus  ||
        //     point10_count != lastpoint10_count||
        //     point5_count  != lastpoint5_count ||
        //     obstacle      != lastObstacle     ||
        //     battery1      != lastbattery1     ||
        //     battery29     != lastbattery29    ||
        //     battery3      != lastbattery3     ||
        //     battery4      != lastbattery4     ||
        //     battery5      != lastbattery5     ||
        //     battery6      != lastbattery6     ||
        //     err_op_count  != last_err_op_count||
        //     sensor0       != lastSensor0      ||
        //     sensor1       != lastSensor1      ||
        //     sensor2       != lastSensor2      ||
        //     sensor3       != lastSensor3)
        if (point10_count != lastpoint10_count||
            point5_count  != lastpoint5_count ||
            obstacle      != lastObstacle     ||
            battery1      != lastbattery1     ||
            battery29     != lastbattery29    ||
            battery3      != lastbattery3     ||
            battery4      != lastbattery4     ||
            battery5      != lastbattery5     ||
            battery6      != lastbattery6     ||
            err_op_count  != last_err_op_count)
        {
            // if (lastpoint10_count != point10_count || lastpoint5_count != point5_count){
            //     global::playSound("sounds/投入中.mp3");//音效
            //     emit idleWatchPoke();
            // }
            global::playSound("sounds/投入中.mp3");//音效
            emit idleWatchPoke();

            // if (obstacle != lastObstacle ){
            //     // 阻塞模式自動關閉寫法（可選）
            //     QTimer::singleShot(3000, err_dlg, &QDialog::accept);
            //     err_dlg->showFullScreen();
            // }

            if (obstacle != lastObstacle ){
                // 阻塞模式自動關閉寫法（可選）
                global::playSound("sounds/退出中.mp3");//音效
                emit idleWatchPoke();
                QTimer::singleShot(3000, err_dlg, &QDialog::accept);
                err_dlg->showFullScreen();
            }
            else
            {
                global::playSound("sounds/投入中.mp3");//音效
                emit idleWatchPoke();
            }


            qDebug() << "馬達狀態:" << motorStatus
                     << "電池1號:" << battery1
                     << "電池2&9號:" << battery29
                     << "電池3號:" << battery3
                     << "電池4號:" << battery4
                     << "電池5號:" << battery5
                     << "電池6號:" << battery6
                     << "釣魚:" << err_op_count
                     << "異物排除:" << obstacle
                     << "感測器狀態:" << "位置" << sensor0 << "長度" <<sensor1 << "高度" << sensor2 << "金屬" << sensor3;
            // 更新ui
            // ui->point10_lb->setText(QString::number(battery9));
            // ui->point5_lb->setText(QString::number(battery34));

            point10_count = battery1 + battery29; // 1 2 9 電池相加
            point5_count  = battery3 + battery4 + battery5 + battery6; // 3 4 5 6 電池相加
            global::carbonReduction() = (point10_count*0.1*3.684/1000) + (point5_count*0.05*3.684/1000);
            int total = point10_count + point5_count;
            ui->batteryTotal_lb->setText(QString::number(total));

            //更新秒數
            startCountdown(60);

            // 更新前一次值
            lastMotorStatus = motorStatus;
            lastbattery1  = battery1;
            lastbattery29 = battery29;
            lastbattery3  = battery3;
            lastbattery4  = battery4;
            lastbattery5  = battery5;
            lastbattery6  = battery6;
            lastObstacle  = obstacle;
            last_err_op_count  = err_op_count;
            lastSensor0     = sensor0;
            lastSensor1     = sensor1;
            lastSensor2     = sensor2;
            lastSensor3     = sensor3;
            lastpoint10_count = point10_count;
            lastpoint5_count = point5_count;
        }

        //馬達賭轉 在showEvent 先設為false
        if(DeviceAPI_GetMotorStalled()>=10)
        {
            global::MotorStalled = true;
            global::monitor()->checkStatus();//馬上檢查
        }

        //馬達異常
        int motorIsErr = DeviceAPI_GetMotorErrorCode(); //0 表示正常
        if(motorIsErr != 0 && motorIsErr != 20)
        {
            qDebug("馬達異常處理開始");
            // global::monitor()->checkStatus();//馬上檢測
            machineErr_Dialog machineErr_dlg(this);
            machineErr_dlg.setText("機台異常");


            // 電池重量轉換減碳量
            double totalWeight = 0.0;
            totalWeight += battery1  * global::batteryRate_1;    // 1號電池
            totalWeight += battery29 * global::batteryRate_2;    // 2號電池
            totalWeight += battery3  * global::batteryRate_3;    // 3號電池
            totalWeight += battery4  * global::batteryRate_4;    // 4號電池
            totalWeight += battery5  * global::batteryRate_5;    // 5號電池
            totalWeight += battery6  * global::batteryRate_6;    // 6號電池
            if (totalWeight > 0.0) {
                global::estWeightG ()+= totalWeight;             // 記憶體累加
                global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double）
            }
            QMap<QString,int> count = collectBatteryStatus();
            // 回傳給後端
            global::mqtt().sendDonationEvent(totalWeight,count,obstacle,true);

            QTimer::singleShot(4000, &machineErr_dlg, &QDialog::accept); // 2 秒後關閉 (也可用 close)
            // 在 stack 上建立，Qt 幫忙管理記憶體


            if (machineErr_dlg.exec() == QDialog::Accepted) {
                QStackedWidget *stack = qobject_cast<QStackedWidget*>(this->parent());
                if (stack) {
                    stack->setCurrentIndex(0);   // 切至首頁
                    // global::getVideoShow()->show();//輪播顯示
                    global::getVideoShow()->hide();//輪播顯示
                    global::monitor()->checkStatus(); //調用檢查函式
                }
            }

            // QStackedWidget *stack = qobject_cast<QStackedWidget*>(this->parent());
            // if (stack) {
            //     stack->setCurrentIndex(0);   // 切至首頁
            //     global::getVideoShow()->show();//輪播顯示
            // }
            return;
        }
    });
}

nmb_PutInBatteryPage::~nmb_PutInBatteryPage()
{
    delete err_dlg;
    qDebug()<<"nmb_PutInBatteryPage 解構err_dlg";
    delete ui;
}


void nmb_PutInBatteryPage::showEvent(QShowEvent *event)
{
    // 暫時放這
    // machineRecord_Dialog machineRecord_Dlg(this);
    // QTimer::singleShot(2000, &machineRecord_Dlg, &QDialog::accept); // 3 秒後關閉 (也可用 close)
    // machineRecord_Dlg.exec();

    if (global::idlewatcher()) global::idlewatcher()->pause();//待機暫停監控
    qDebug()<<"進入非會員放入頁面讀入當前值累積之重量"<<global::estWeightG();
    video_player->play();//教學影片播放

    ui->station_lb->setText(global::storeInfo);//站點

    global::MotorStalled = false;//馬達堵轉
    // gpio.setState(ledOn); //板子控制燈
    // DeviceAPI_Open(); //開啟裝置

    // DeviceAPI_Run(); //開啟裝置
    qDebug()<< " 異常物 "<<DeviceAPI_GetBattery(0);//異物

    // global::motorstatus = DeviceAPI_Run(); //開啟裝置收電池
    // global::monitor()->checkStatus();

    // LedAPI_Run();
    startCountdown(60);

    // 確認偵測是否變化
    // motorStatus = 0; //馬達狀態
    // point10_count  = 0;
    // point5_count   = 0;
    // battery1    = 0; //1號電池數
    // battery29    = 0; //2&9號電池數
    // battery3   = 0; //3號電池數
    // battery4    = 0; //4號電池數
    // battery5    = 0; //5號電池數
    // battery6    = 0; //6號電池數
    // err_op_count = 0;// 釣魚次數 3次直接登出
    // obstacle    = 0;//異物數量
    // sensor0     = 0;  //位置感測值
    // sensor1     = 0;  //長度感測值
    // sensor2     = 0;  //高度感測值
    // sensor3     = 0;  //金屬感測值

    // lastpoint10_count = 0;
    // lastpoint5_count  = 0;
    // lastMotorStatus = 0;
    // lastbattery1  = 0;
    // lastbattery29 = 0;
    // lastbattery3  = 0;
    // lastbattery4  = 0;
    // lastbattery5  = 0;
    // lastbattery6  = 0;
    // lastObstacle  = 0;
    // last_err_op_count  = 0;
    // lastSensor0     = 0;
    // lastSensor1     = 0;
    // lastSensor2     = 0;
    // lastSensor3     = 0;

    // 開始偵測
    if (timer1 && !timer1->isActive())
        timer1->start(100);    // 0.1 秒更新
}

void nmb_PutInBatteryPage::hideEvent(QHideEvent *event)
{

    video_player->pause();//教學影片暫停

    if (global::idlewatcher()) global::idlewatcher()->resume();//待機恢復監控

    // 確認偵測是否變化
    motorStatus = 0; //馬達狀態
    point10_count  = 0;
    point5_count   = 0;
    battery1    = 0; //1號電池數
    battery29    = 0; //2&9號電池數
    battery3   = 0; //3號電池數
    battery4    = 0; //4號電池數
    battery5    = 0; //5號電池數
    battery6    = 0; //6號電池數
    err_op_count = 0;// 釣魚次數 3次直接登出
    obstacle    = 0;//異物數量
    sensor0     = 0;  //位置感測值
    sensor1     = 0;  //長度感測值
    sensor2     = 0;  //高度感測值
    sensor3     = 0;  //金屬感測值

    lastpoint10_count = 0;
    lastpoint5_count  = 0;
    lastMotorStatus = 0;
    lastbattery1  = 0;
    lastbattery29 = 0;
    lastbattery3  = 0;
    lastbattery4  = 0;
    lastbattery5  = 0;
    lastbattery6  = 0;
    lastObstacle  = 0;//異物從0開始
    last_err_op_count  = 0;
    lastSensor0     = 0;
    lastSensor1     = 0;
    lastSensor2     = 0;
    lastSensor3     = 0;

    // DeviceAPI_Close();
    DeviceAPI_Run_Stop(); //關閉收電池
    //LedAPI_Set(0xff0000); //綠燈
    LedAPI_Select(2);
    // LedAPI_Set(0x000000);
    countDownTimer->stop();
    timer1->stop();
    // ui->batteryTotal_lb->setText("00000");
    ui->batteryTotal_lb->setText("0");


}

void nmb_PutInBatteryPage::mousePressEvent(QMouseEvent *event)
{
    startCountdown(60);
}

void nmb_PutInBatteryPage::startCountdown(int seconds)
{
    countdownValue = seconds;
    ui->countdown_lb->setText(QString::number(countdownValue));
    // 確保計時器不會重複連線
    disconnect(countDownTimer, nullptr, this, nullptr);
    connect(countDownTimer, &QTimer::timeout, this, &nmb_PutInBatteryPage::updateCountLabel);
    countDownTimer->start(1000);  // 每 1 秒觸發一次
}

void nmb_PutInBatteryPage::updateCountLabel()
{
    countdownValue -= 1;
    if(countdownValue <= -1 )
    {
        // 電池重量轉換減碳量
        double totalWeight = 0.0;
        totalWeight += battery1  * global::batteryRate_1;    // 1號電池
        totalWeight += battery29 * global::batteryRate_2;    // 2號電池
        totalWeight += battery3  * global::batteryRate_3;    // 3號電池
        totalWeight += battery4  * global::batteryRate_4;    // 4號電池
        totalWeight += battery5  * global::batteryRate_5;    // 5號電池
        totalWeight += battery6  * global::batteryRate_6;    // 6號電池

        global::carbonReduction() = totalWeight * 3.684;//碳量單位克
        if (totalWeight > 0.0) {
            global::estWeightG ()+= totalWeight;                    // 記憶體累加
            global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double

            global::total_carbonReduction += global::carbonReduction(); //機台碳足跡
            global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
        }

        // global::estWeightG += totalWeight;//重量公克
        // global::writeConfig("totalWeight", global::estWeightG);//更新累積重量

        QMap<QString,int> count = collectBatteryStatus();
        bool disconnect = global::monitor()->checkNetwork();//true表示網路斷線
        // 回傳給後端
        global::mqtt().sendDonationEvent(totalWeight,count,obstacle,true);
        //做跳轉
        countDownTimer->stop();
        // DeviceAPI_Close(); // 關閉裝置
        // LedAPI_Select(0);
        QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
        if (stack) {
            stack->setCurrentIndex(0);   // 切至首頁
            // global::getVideoShow()->start();//輪播開始
            global::getVideoShow()->show();//輪播隱藏
        }
    }
    else
    {
        ui->countdown_lb->setText(QString::number(countdownValue));
    }
}


void nmb_PutInBatteryPage::on_makesure_btn_clicked()
{
    // DeviceAPI_Close();          // 關閉裝置
    // LedAPI_Select(0);

    // 電池重量轉換減碳量
    double totalWeight = 0.0;
    totalWeight += battery1  * global::batteryRate_1;    // 1號電池
    totalWeight += battery29 * global::batteryRate_2;    // 2號電池
    totalWeight += battery3  * global::batteryRate_3;    // 3號電池
    totalWeight += battery4  * global::batteryRate_4;    // 4號電池
    totalWeight += battery5  * global::batteryRate_5;    // 5號電池
    totalWeight += battery6  * global::batteryRate_6;    // 6號電池

    global::carbonReduction() = totalWeight * 3.684;//碳量單位克
    qDebug()<<"目前"<<global::estWeightG();

    if (totalWeight > 0.0) {
        global::estWeightG ()+= totalWeight;                    // 記憶體累加
        global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double）

        global::total_carbonReduction += global::carbonReduction(); //機台碳足跡
        global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
    }
    qDebug()<<"加了後"<<global::estWeightG();
    // global::estWeightG += totalWeight;//重量 換算公斤
    // global::writeConfig("totalWeight", global::estWeightG);//更新累積重量

    QMap<QString,int> count = collectBatteryStatus();
    bool disconnect = global::monitor()->checkNetwork();//true表示網路斷線
    // 回傳給後端
    global::mqtt().sendDonationEvent(totalWeight,count,obstacle,true);


    QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
    if (stack) {
        stack->setCurrentIndex(5);   // 切至非會員完成頁面
        // global::getVideoShow()->start();//輪播開始
        global::getVideoShow()->show();//輪播開始
    }
}

QMap<QString,int> nmb_PutInBatteryPage::collectBatteryStatus() const {
    QMap<QString,int> map;
    map.insert("1",  lastbattery1);
    map.insert("2",  lastbattery29);
    map.insert("3",  lastbattery3);
    map.insert("4",  lastbattery4);
    map.insert("5",  lastbattery5);
    map.insert("6",  lastbattery6);
    map.insert("9",  0);//原本的
    //map.insert("9",  DeviceAPI_GetMotorStalled());//9用來放賭轉次數
    return map;
}
