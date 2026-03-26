#include "mb_loginpage.h"
#include "ui_mb_loginpage.h"
#include "clockbus.h"
#include <QStackedWidget>
#include <QDebug>
#include "welcom_dialog.h"
#include "loginerr_dialog.h"
#include "global.h"
#include "machineerr_dialog.h"
#include "DeviceAPI.h"

mb_LoginPage::mb_LoginPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mb_LoginPage)
{
    ui->setupUi(this);
    countDownTimer = new QTimer(this);
    // ui->remind_lb->setText("請輸入手機號碼登入");
    // PlayTool *playwindow = &global::videoShow();
    // playwindow->setParent(this);  // 或你的 host 容器
    // playwindow->setWindowFlags(Qt::Widget);
    // // const QRect g = centralWidget()->rect();
    // //playwindow->setGeometry(0, 0, g.width(), g.height() / 2);
    // playwindow->raise();
    // playwindow->show();
    // playwindow->start();



    connect(ClockBus::instance(),&ClockBus::minuteTick,this,[this](const QDateTime&, const QString& s) //使用全域變數QTimer每分鐘更新時間
            {
                ui->time_lb->setText(s);
            });

    connect(&global::mqtt(), &MqttHelper::loginAckReceived,
            this, &mb_LoginPage::onLoginAckReceived);

    setConnect();//連接按鈕

}


mb_LoginPage::~mb_LoginPage()
{
    delete ui;
}

void  mb_LoginPage::showEvent(QShowEvent *event)
{
    login_is = false;
    ui->inpute_edit->setText("");
    ui->station_lb->setText(global::storeInfo);//站點
    setOriginalText();
    startCountdown(30);//開始計時及設置計時

}

void  mb_LoginPage::hideEvent(QHideEvent *event)
{
    countDownTimer->stop();
}

void mb_LoginPage::setConnect()
{
    for (int i = 0; i <= 9; i++) {
        QString name = QString("number_btn_%1").arg(i);   // 和你的物件名一致
        QPushButton *btn = findChild<QPushButton*>(name);
        if (btn) {
            connect(btn, &QPushButton::clicked, this, [this, i] {
                global::playSound("sounds/點擊.mp3");
                onDigitClicked(i);
                startCountdown(30);
            });
        }
    }
}

void mb_LoginPage:: onDigitClicked(int digitValue)
{
    // ui->inpute_edit->insert(QString::number(digitValue));
    if (ui->inpute_edit->text().size() >= 10) {
        // QApplication::beep();                    // 超過就提示
        // 可選：若有 statusbar
        // ui->statusbar->showMessage("最多 10 碼", 1500);
        return;
    }
    ui->inpute_edit->insert(QString::number(digitValue));
    setOriginalText();
    // inputTimer.start(3000);
}

void mb_LoginPage::on_clear_btn_clicked()
{
    ui->inpute_edit->backspace();
    setOriginalText();
    startCountdown(30);
    global::playSound("sounds/返回.mp3");
    // inputTimer.start(3000);
}

void mb_LoginPage::setErrorText()
{
    ui->remind_lb->setStyleSheet(
        "QLabel {"
        "   font-family: 'Noto Sans CJK TC';"
        "   font-size: 28px;"
        "   font-weight: 500;"        // normal / bold / 100~900
        "   color: #FF5000;"            // 可加上字色
        "   border: none;"
        "   background-color: transparent;"
        "}"
        );
    ui->remind_lb->setText("輸入錯誤，請重新輸入");
    ui->remind_lb->adjustSize();
}

void mb_LoginPage::setOriginalText()
{
    ui->remind_lb->setStyleSheet(
        "QLabel {"
        "   font-family: 'Noto Sans CJK TC';"
        "   font-size: 28px;"
        "   font-weight: 500;"        // normal / bold / 100~900
        "   color: black;"            // 可加上字色
        "   border: none;"
        "   background-color: transparent;"
        "}"
        );
    ui->remind_lb->setText("請輸入手機號碼登入");
    ui->remind_lb->adjustSize();
}


void mb_LoginPage::updateCountLabel()
{
    countdownValue -= 1;
    if(countdownValue <= -1 )
    {
        //做跳轉
        countDownTimer->stop();
        QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
        if (stack) {
            stack->setCurrentIndex(0);   // 切至首頁
        }
    }
    else
    {
        ui->countdown_lb->setText(QString::number(countdownValue));
    }
}

void mb_LoginPage:: startCountdown(int seconds)
{
    countdownValue = seconds;
    ui->countdown_lb->setText(QString::number(countdownValue));
    // 確保計時器不會重複連線
    disconnect(countDownTimer, nullptr, this, nullptr);
    connect(countDownTimer, &QTimer::timeout, this, &mb_LoginPage::updateCountLabel);
    countDownTimer->start(1000);  // 每 1 秒觸發一次
}

void mb_LoginPage::on_back_btn_clicked()
{
    global::playSound("sounds/返回.mp3");
    QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
    if (stack) {
        stack->setCurrentIndex(0);   // 切回 HomePage
    }
}


void mb_LoginPage::mousePressEvent(QMouseEvent *event)
{
    startCountdown(30);
}

void mb_LoginPage::onLoginAckReceived(const QJsonObject &payload)
{
    if (!payload.contains("loginAck") || !payload["loginAck"].isObject())
        return;

    QJsonObject loginAck = payload["loginAck"].toObject();
    QString code = loginAck.value("code").toString();

    if (login_is){
        if (code == "OK") {
            // 登入成功才切頁
            global::username = loginAck.value("maskedName").toString();
            qDebug() << "User:" << global::username << "登入成功" ;
            login_is = false;
            global::playSound("sounds/登入成功.mp3");
            welcom_Dialog wel_dlg(this);
            QTimer::singleShot(2000, &wel_dlg, &QDialog::accept); // 3 秒後關閉 (也可用 close)
            wel_dlg.exec();


            // welcom_Dialog* wel_dlg = new welcom_Dialog(this);          // ✅ 非阻塞
            // wel_dlg->setAttribute(Qt::WA_DeleteOnClose);
            // wel_dlg->show();
            // QTimer::singleShot(2000, wel_dlg, &QDialog::accept);

            // // ✅ 修正：將頁面切換的邏輯連接到對話框的 accepted() 訊號
            // connect(wel_dlg, &welcom_Dialog::accepted, this, [this](){
            //     QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
            //     if (stack) {
            //         stack->setCurrentIndex(2); // 在對話框被接受後，才切換頁面
            //     }
            // });

            global::motorstatus = DeviceAPI_Run(); //開啟裝置收電池

            if(global::motorstatus == 0)//正常時
            {
                QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
                if (stack) {
                    stack->setCurrentIndex(2);   // 切至會員投入電池頁面
                    global::getVideoShow()->stop();// 1. 暫停並隱藏影片輪播
                    global::getVideoShow()->hide();
                }
            }else
            {
                global::monitor()->checkStatus();
            }

            //原本只有這個
            // QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
            // if (stack) {
            //     stack->setCurrentIndex(2);   // 切至會員投入電池頁面
            //     global::getVideoShow()->stop();// 1. 暫停並隱藏影片輪播
            //     global::getVideoShow()->hide();
            // }

        } else {
            // 登入失敗，顯示錯誤訊息
            qDebug() << "登入失敗" ;
            global::playSound("sounds/登入失敗.mp3");

            global::isLoginfalse = true; //設為true表示登入失敗
            global::monitor()->checkStatus();//馬上檢查
            global::isLoginfalse = false;//檢查完馬上恢復狀態避免下次在檢測到

            loginErr_Dialog loginErr_dlg(this);
            QTimer::singleShot(2000, &loginErr_dlg, &QDialog::accept); // 3 秒後關閉 (也可用 close)
            loginErr_dlg.exec();
            // 這裡顯示錯誤
            setErrorText();
        }
    }

}

void mb_LoginPage::on_makesure_btn_clicked()
{
    if (ui->inpute_edit->text().size() < 10)
    {
        global::playSound("sounds/登入失敗.mp3");
        loginErr_Dialog loginErr_dlg(this);
        QTimer::singleShot(2000, &loginErr_dlg, &QDialog::accept); // 3 秒後關閉 (也可用 close)
        loginErr_dlg.exec();
        // 這裡顯示錯誤
        setErrorText();
        return;
    }

    bool disconnect = global::monitor()->checkNetwork();//true表示網路斷線
    // int motorIsErr = DeviceAPI_GetMotorErrorCode(); //0 表示正常
    //網路異常 、馬達異常、mqtt未連接 則無法登入
    if(disconnect /*|| motorIsErr != 0 */|| global::mqtt().m_client->state() != QMqttClient::Connected)
    {
        global::playSound("sounds/登入失敗.mp3");
        global::monitor()->checkStatus();//馬上檢測
        machineErr_Dialog machineErr_dlg(this);
        QTimer::singleShot(2000, &machineErr_dlg, &QDialog::accept); // 2 秒後關閉 (也可用 close)
        machineErr_dlg.exec();
        return;
    }

    global::phonenumber = ui->inpute_edit->text();
    global::loginTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    global::mqtt().sendLoginEvent("PHONE", ui->inpute_edit->text());
    login_is = true;

    //防連點
    ui->makesure_btn->setEnabled(false);
    ui->makesure_btn->setText("稍候3秒");

    QTimer::singleShot(3000, this, [this]() {
        ui->makesure_btn->setEnabled(true);
        ui->makesure_btn->setText("確定");
    });

    // global::playSound("sounds/登入成功.mp3");
    // welcom_Dialog welcom_dlg(this);               // parent = 子視窗
    // QTimer::singleShot(2000, &welcom_dlg, &QDialog::accept); // 3 秒後關閉 (也可用 close)
    // welcom_dlg.exec();

    // QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
    // if (stack) {
    //     stack->setCurrentIndex(2);   // 切至會員投入電池頁面
    // }
}

