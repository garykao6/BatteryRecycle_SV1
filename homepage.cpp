#include "homepage.h"
#include "ui_homepage.h"
#include "clockbus.h"
#include "service_dialog.h"
#include "global.h"
#include "DeviceAPI.h"
#include "machineerr_dialog.h"
HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HomePage)
{
    ui->setupUi(this);
    // PlayTool *playwindow = new PlayTool(this);
    // playwindow->setGeometry(0,0,1080,1120);
    // playwindow->show();
    // playwindow->start();

    // PlayTool *playwindow = &global::videoShow();
    // playwindow->setParent(this);  // 或你的 host 容器
    // playwindow->setWindowFlags(Qt::Widget);
    // // const QRect g = centralWidget()->rect();
    // //playwindow->setGeometry(0, 0, g.width(), g.height() / 2);
    // playwindow->raise();
    // playwindow->show();
    // playwindow->start();

    // global::videoShow().setGeometry(0,0,1080,1120);
    // global::videoShow().show();
    // global::videoShow().start();

    //關閉按鈕上面的label接收滑鼠事件避免按鈕無法點擊
    ui->showtxt_1_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->showtxt_2_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->showtxt_3_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->showtxt_4_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->label_31->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->label_32->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    connect(ClockBus::instance(),&ClockBus::minuteTick,this,[this](const QDateTime&, const QString& s) //使用全域變數QTimer每分鐘更新時間
            {
        ui->time_lb->setText(s);
    });

    connect(&global::mqtt(), &MqttHelper::storeInfoAckReceived,this, &HomePage::onstoreInfoAckReceived);

    //LedAPI_Run();
    //LedAPI_Set(0xff0000);
    LedAPI_Select(2);

    //長按退出程式
    connect(ui->pushButton, &QPushButton::pressed, this, [this]{
        // 按下去 → 開始計時 7 秒
        QTimer::singleShot(7000, this, [this]{
            if (ui->pushButton->isDown()) {  // 確認還在按住
                qDebug() << "👋 長按3秒觸發，程式即將正常退出";
                //LedAPI_Close();
                DeviceAPI_Close();
                QCoreApplication::quit();
            }
        });
    });

}

HomePage::~HomePage()
{
    delete ui;
}

void HomePage::showEvent(QShowEvent *event)
{
    global::reset();
    //LedAPI_Set(0xff0000);//green
    LedAPI_Select(2);

    global::total_carbonReduction = global::readConfigValue("total_carbonReduction","0").toDouble();
    global::mqtt().sendstoreInfo();//發送取的站點名稱訊號
    ui->carbon_lb->setText(QString::number(global::total_carbonReduction/1000, 'f', 1));
    // ui->station_lb->setText(global::storeInfo);
    // Relay1API_On(); //開頭頂燈
    // LedAPI_Select(0);
}

void HomePage::on_mb_btn_clicked()
{
    global::playSound("sounds/確認.mp3");
    QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
    if (stack) {
        stack->setCurrentIndex(1);   //切至會員登錄頁面
    }
}

void HomePage::on_nmb_btn_clicked()
{
    global::playSound("sounds/確認.mp3");
    // bool disconnect = global::monitor()->checkNetwork();//true表示網路斷線
    // int motorIsErr = DeviceAPI_GetMotorErrorCode(); //0 表示正常
    // if(motorIsErr != 0)
    // {
    //     global::monitor()->checkStatus();//馬上檢測
    //     machineErr_Dialog machineErr_dlg(this);
    //     machineErr_dlg.setText("機台異常,無法使用");
    //     QTimer::singleShot(2000, &machineErr_dlg, &QDialog::accept); // 2 秒後關閉 (也可用 close)
    //     machineErr_dlg.exec();
    //     return;
    // }


    global::motorstatus = DeviceAPI_Run(); //開啟裝置收電池

    if(global::motorstatus == 0)//正常
    {
        QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
        if (stack) {
            stack->setCurrentIndex(3);   // 切至非會員投放頁面
            // global::getVideoShow()->stop();// 1. 暫停並隱藏影片輪播
            global::getVideoShow()->hide();
        }
    }else
    {
        global::monitor()->checkStatus();
    }



    // QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
    // if (stack) {
    //     stack->setCurrentIndex(3);   // 切至非會員投放頁面
    //     global::getVideoShow()->stop();// 1. 暫停並隱藏影片輪播
    //     global::getVideoShow()->hide();
    // }
}

void HomePage::on_service_btn_clicked()
{
    global::playSound("sounds/確認.mp3");
    service_Dialog sv_dlg(this);
    sv_dlg.exec();
}


void HomePage::on_pushButton_clicked()
{
    // qDebug() << "👋 程式即將正常退出";
    // LedAPI_Close();
    // DeviceAPI_Close();
    // QCoreApplication::quit();
}

//進入工程頁面
void HomePage::on_go_backstage_login_btn_clicked()
{

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastClickTime > 3000) clickCount = 0;
    lastClickTime = now;
    ++clickCount;

    qDebug() << "點擊次數:" << clickCount;

    if (clickCount < 7) return;
    clickCount = 0;

    // ---- 這裡開始切頁 ----
    QStackedWidget* stack_1 = qobject_cast<QStackedWidget*>(parent());        // 用成員，不用 parentWidget()
    if (!stack_1) { qWarning() << "❌ 找不到父 stack（成員為空）"; return;}

    constexpr int kBackIndex = 8; //8                      // 目標頁索引
    const int n = stack_1->count();
    qDebug() << "stack =" << stack_1 << "count =" << n << "target =" << kBackIndex;

    if (kBackIndex < 0 || kBackIndex >= n) {
        qWarning() << "❌ 目標索引不存在：" << kBackIndex << "(count =" << n << ")";
        return;
    }
    stack_1->setCurrentIndex(kBackIndex);                  // 用索引最安全
    global::getVideoShow()->stop();// 1. 暫停並隱藏影片輪播
    global::getVideoShow()->hide();
    qDebug() << "✅ 已切到 index" << kBackIndex;
}

//解析取得站點名稱
void HomePage::onstoreInfoAckReceived(const QJsonObject& obj)
{
    const QJsonObject storeObj = obj.value("store").toObject();
    const QString storeName = storeObj.value("name").toString();

    if (!storeName.isEmpty()) {
        qDebug() << "✅ 站點名稱:" << storeName;
        global::storeInfo = storeName;
        ui->station_lb->setText(global::storeInfo);
    } else {
        qDebug() << "⚠️ store 裡沒有 name 欄位";
    }
}
