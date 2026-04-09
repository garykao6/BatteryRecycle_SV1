#include "mb_putinbatterypage.h"
#include "global.h"
#include "ui_mb_putinbatterypage.h"
#include "clockbus.h"
#include <QStackedWidget>
#include "DeviceAPI.h"
#include "global.h" //音效
// #include "machineerr_dialog.h"
#include "pointupload_dialog.h"
#include "machinerecord_dialog.h"//回收點數異常UI
#include "machineerr_dialog.h"//機台異常

mb_PutINBatteryPage::mb_PutINBatteryPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mb_PutINBatteryPage)
    ,gpio(5) // 使用 GPIO5
{
    ui->setupUi(this);
    countDownTimer = new QTimer(this);
    ui->showtxt_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);

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



    // 每秒更新時間
    connect(ClockBus::instance(),&ClockBus::minuteTick,this,[this](const QDateTime&, const QString& s) //使用全域變數QTimer每分鐘更新時間
            {
                ui->time_lb->setText(s);
            });

    // ======= 新增：每0.1秒更新馬達/電池/感測器 =======
    timer1 = new QTimer(this);
    connect(timer1, &QTimer::timeout, this, [this]() {

        // if(DeviceAPI_GetMotorErrorCode()!= 0)
        // {
        //     return;
        // }


        //emit idleWatchPoke();

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

        // // 如果有任一個值改變就印出
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
        // 如果有任一個值改變就印出
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
            point10_count = battery1 + battery29; // 1 2 9 電池相加
            point5_count  = battery3 + battery4 + battery5 + battery6; // 3 4 5 6 電池相加
            ui->point10_lb->setText(QString::number(point10_count));
            ui->point5_lb->setText(QString::number(point5_count));

            int total = 10 * point10_count + 5 *point5_count;
            ui->pointtotal_lb->setText(QString::number(total));

            //更新秒數
            startCountdown(60);
            // mousePressEvent(event);

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

            //釣魚機制
            if(err_op_count>=5)
            {
                qDebug()<<"觸發釣魚機制";
                global::isPhishing = true; //設為true表示在釣魚
                global::monitor()->checkStatus();//馬上檢查
                global::isPhishing = false;//檢查完馬上恢復狀態避免下次在檢測到

                // 電池重量轉換減碳量
                double totalWeight = 0.0;
                totalWeight += battery1  * global::batteryRate_1;    // 1號電池
                totalWeight += battery29 * global::batteryRate_2;    // 2號電池
                totalWeight += battery3  * global::batteryRate_3;    // 3號電池
                totalWeight += battery4  * global::batteryRate_4;    // 4號電池
                totalWeight += battery5  * global::batteryRate_5;    // 5號電池
                totalWeight += battery6  * global::batteryRate_6;    // 6號電池
                if (totalWeight > 0.0) {
                    global::estWeightG ()+= totalWeight;                    // 記憶體累加
                    global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double）
                }
                global::points = ui->pointtotal_lb->text();
                QMap<QString,int> count = collectBatteryStatus();
                // 回傳給後端
                global::mqtt().sendRecycleEvent(totalWeight, global::points.toInt(), false,                               // donation
                                                count,                               // count 物件
                                                "PHONE",                             // loginMethod
                                                global::phonenumber,lastObstacle);
                // ================電池重量轉換減碳量=============

                //馬上登出
                QStackedWidget *stack = qobject_cast<QStackedWidget*>(this->parent());
                if (stack) {
                    stack->setCurrentIndex(0);   // 切至首頁
                    // global::getVideoShow()->start();//輪播開始
                    global::getVideoShow()->show();//輪播顯示
                }
            }
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
                global::estWeightG ()+= totalWeight;                    // 記憶體累加
                global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double）
            }
            global::points = ui->pointtotal_lb->text();
            QMap<QString,int> count = collectBatteryStatus();
            // 回傳給後端
            global::mqtt().sendRecycleEvent(totalWeight, global::points.toInt(), false,                               // donation
                                            count,                               // count 物件
                                            "PHONE",                             // loginMethod
                                            global::phonenumber,lastObstacle);

            QTimer::singleShot(4000, &machineErr_dlg, &QDialog::accept); // 2 秒後關閉 (也可用 close)


            if (machineErr_dlg.exec() == QDialog::Accepted) {
                QStackedWidget *stack = qobject_cast<QStackedWidget*>(this->parent());
                if (stack) {
                    stack->setCurrentIndex(0);   // 切至首頁
                    // global::getVideoShow()->show();//輪播顯示
                    // global::getVideoShow()->stop();//輪播顯示
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
    // timer1->start(100);  // 每0.1秒更新一次
}

mb_PutINBatteryPage::~mb_PutINBatteryPage()
{
    delete err_dlg;
    qDebug()<<"mb_PutINBatteryPage 解構err_dlg";
    delete ui;
}

void mb_PutINBatteryPage::showEvent(QShowEvent *event)
{
    // 暫時放這
    // machineErr_Dialog machineErr_dlg(this);
    // QTimer::singleShot(2000, &machineErr_dlg, &QDialog::accept); // 3 秒後關閉 (也可用 close)
    // machineErr_dlg.exec();


    // pointUpload_Dialog pu_dlg(this);
    // QTimer::singleShot(2000, &pu_dlg, &QDialog::accept); // 3 秒後關閉 (也可用 close)
    // pu_dlg.exec();

    // QWidget::showEvent(event);  // 保留父類行為

    // QTimer::singleShot(2000,err_dlg,&ErrorItem_Dialog::close);
    // err_dlg->show();

    if (global::idlewatcher()) global::idlewatcher()->pause();//待機暫停監控

    qDebug()<<"進入會員放入頁面讀入當前值累積之重量"<<global::estWeightG();

    video_player->play();//教學影片播放

    ui->mbName_lb->setText(global::username);
    ui->station_lb->setText(global::storeInfo);//站點
    global::MotorStalled = false; //馬達堵轉
    // gpio.setState(ledOn); //板子控制燈
    // DeviceAPI_Open(); //開啟裝置

    // DeviceAPI_Run(); //開啟裝置

    //移到前一頁 因為在這裡馬上檢查不會跳維護頁面
    // global::motorstatus = DeviceAPI_Run(); //開啟裝置收電池
    // global::monitor()->checkStatus();

    qDebug()<< " 異常物 "<<DeviceAPI_GetBattery(0);//異物
    // LedAPI_Run();
    startCountdown(60);

    // 確認偵測是否變化
    // lastMotorStatus = -1;
    // lastBattery34   = -1;
    // lastBattery9    = -1;
    // lastObstacle    = -1;
    // lastSensor0     = -1;
    // lastSensor1     = -1;
    // lastSensor2     = -1;
    // lastSensor3     = -1;


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

    // lastpoint10_count = -1;
    // lastpoint5_count  = -1;
    // lastMotorStatus = -1;
    // lastbattery1  = -1;
    // lastbattery29 = -1;
    // lastbattery3  = -1;
    // lastbattery4  = -1;
    // lastbattery5  = -1;
    // lastbattery6  = -1;
    // lastObstacle  = 0;//異物從0開始
    // last_err_op_count  = -1;
    // lastSensor0     = -1;
    // lastSensor1     = -1;
    // lastSensor2     = -1;
    // lastSensor3     = -1;

    // 開始偵測
    if (timer1 && !timer1->isActive())
        timer1->start(100);    // 0.1 秒更新
}

void mb_PutINBatteryPage::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);  // 保留父類行為

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

    video_player->pause();//教學影片暫停
    countDownTimer->stop();
    timer1->stop();
    // DeviceAPI_Close();// 關閉裝置
    DeviceAPI_Run_Stop();// 關閉裝置
    //LedAPI_Set(0xff0000); //綠燈
    LedAPI_Select(2);

    // LedAPI_Set(0x000000);
    // ui->point10_lb->setText("00000");
    // ui->point5_lb->setText("00000");
    // ui->pointtotal_lb->setText("00000000");
    ui->point10_lb->setText("0");
    ui->point5_lb->setText("0");
    ui->pointtotal_lb->setText("0");
}

void mb_PutINBatteryPage::mousePressEvent(QMouseEvent *event)
{
    startCountdown(60);
}

void mb_PutINBatteryPage::startCountdown(int seconds)
{
    countdownValue = seconds;
    ui->countdown_lb->setText(QString::number(countdownValue));
    // 確保計時器不會重複連線
    disconnect(countDownTimer, nullptr, this, nullptr);
    connect(countDownTimer, &QTimer::timeout, this, &mb_PutINBatteryPage::updateCountLabel);
    countDownTimer->start(1000);  // 每 1 秒觸發一次
}

void mb_PutINBatteryPage::updateCountLabel()
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

        global::carbonReduction() = totalWeight * 3.684; //碳量
        if (totalWeight > 0.0) {
            global::estWeightG ()+= totalWeight;                    // 記憶體累加
            global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double）

            global::total_carbonReduction += global::carbonReduction(); //機台碳足跡
            global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
        }
        global::points = ui->pointtotal_lb->text();
        // global::writeConfig("totalWeight", global::estWeightG);//更新累積重量

        QMap<QString,int> count = collectBatteryStatus();
        bool disconnect = global::monitor()->checkNetwork();//true表示網路斷線
        // 回傳給後端
        global::mqtt().sendRecycleEvent(totalWeight, global::points.toInt(), false,                               // donation
                                        count,                               // count 物件
                                        "PHONE",                             // loginMethod
                                        global::phonenumber,lastObstacle);
        //做跳轉
        countDownTimer->stop();
        // DeviceAPI_Close();          // 關閉裝置
        // LedAPI_Select(0);
        QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
        if (stack) {
            stack->setCurrentIndex(0);   // 切至首頁
            // global::getVideoShow()->start();//輪播開始 不使用 跳轉後會沒有影片
            global::getVideoShow()->show();//輪播顯示
        }
    }
    else
    {
        ui->countdown_lb->setText(QString::number(countdownValue));
    }
}

void mb_PutINBatteryPage::on_makesure_btn_clicked()
{
    // DeviceAPI_Close();          // 關閉裝置
    // LedAPI_Select(0);
    countDownTimer->stop();
    // 關閉偵測
    if (timer1 && timer1->isActive())
        timer1->stop();         // 停止 0.1 秒更新

    // 電池重量轉換減碳量
    double totalWeight = 0.0;//克
    totalWeight += battery1  * global::batteryRate_1;    // 1號電池
    totalWeight += battery29 * global::batteryRate_2;    // 2號電池
    totalWeight += battery3  * global::batteryRate_3;    // 3號電池
    totalWeight += battery4  * global::batteryRate_4;    // 4號電池
    totalWeight += battery5  * global::batteryRate_5;    // 5號電池
    totalWeight += battery6  * global::batteryRate_6;    // 6號電池


    // global::carbonReduction() = (point10_count*0.1*3.684/1000) + (point5_count*0.05*3.684/1000);
    global::carbonReduction() = totalWeight * 3.684;//碳量
    qDebug()<<"目前"<<global::estWeightG();
    // ③ 只有「真的有重量」才更新累積與寫檔
    if (totalWeight > 0.0) {
        global::estWeightG ()+= totalWeight;                    // 記憶體累加
        global::writeConfig("totalWeight", global::estWeightG()); // 寫回 JSON（double）

        global::total_carbonReduction += global::carbonReduction(); //機台碳足跡
        global::writeConfig("total_carbonReduction", global::total_carbonReduction); //機台碳足跡寫回JSON
    }
    global::points = ui->pointtotal_lb->text();
    qDebug()<<"加了後"<<global::estWeightG();

    // 1️⃣ 準備電池種類數量
    QMap<QString,int> count = collectBatteryStatus();
    bool disconnect = global::monitor()->checkNetwork();//true表示網路斷線
    if(disconnect || global::mqtt().m_client->state() != QMqttClient::Connected)
    {
        global::playSound("sounds/機台異常點數匯入.mp3");
        machineRecord_Dialog mr_dlg(this);
        QTimer::singleShot(4000, &mr_dlg, &QDialog::accept); // 2 秒後關閉 (也可用 close)
        mr_dlg.exec();
    }

    // 回傳給後端
    global::mqtt().sendRecycleEvent(totalWeight, global::points.toInt(), false,                               // donation
                                    count,                               // count 物件
                                    "PHONE",                             // loginMethod
                                    global::phonenumber,
                                    lastObstacle);

    if(!disconnect && global::mqtt().m_client->state() == QMqttClient::Connected)
    {
        global::playSound("sounds/點數匯入.mp3");
        pointUpload_Dialog pu_dlg(this);
        QTimer::singleShot(2000, &pu_dlg, &QDialog::accept); // 2 秒後關閉 (也可用 close)
        pu_dlg.exec();
    }

    // global::playSound("sounds/點數匯入.mp3");
    // pointUpload_Dialog pu_dlg(this);
    // QTimer::singleShot(2000, &pu_dlg, &QDialog::accept); // 2 秒後關閉 (也可用 close)
    // pu_dlg.exec();

    QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
    if (stack) {
        stack->setCurrentIndex(4);   // 切至會員完成頁面
        // global::getVideoShow()->start();//輪播開始
        global::getVideoShow()->show();//輪播顯示
    }
}

QMap<QString,int> mb_PutINBatteryPage::collectBatteryStatus() const {
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
