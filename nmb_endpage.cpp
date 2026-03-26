#include "nmb_endpage.h"
#include "ui_nmb_endpage.h"
#include "clockbus.h"
#include <QStackedWidget>
#include "global.h"
nmb_endpage::nmb_endpage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::nmb_endpage)
{
    ui->setupUi(this);
    ui->showtxt_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);//使該label不能接收滑鼠事件

    countDownTimer = new QTimer(this);

    connect(ClockBus::instance(),&ClockBus::minuteTick,this,[this](const QDateTime&, const QString& s) //使用全域變數QTimer每分鐘更新時間
            {
                ui->time_lb->setText(s);
            });
}

nmb_endpage::~nmb_endpage()
{
    delete ui;
}

void nmb_endpage::showEvent(QShowEvent *event)
{
    qDebug()<<"碳量"<<global::carbonReduction();
    ui->station_lb->setText(global::storeInfo);//站點

    if (global::carbonReduction()  < 1000) {
        // 小於 1000 → 顯示「克」
        ui->carbon_lb->setText(QString::number(global::carbonReduction() , 'f', 1));
        ui->kg_lb->setText("克");
    }
    else if (global::carbonReduction()  < 1000000) {
        // 1000 g ~ 999999 g → 顯示「公斤」
        double kg = global::carbonReduction()  / 1000.0;
        ui->carbon_lb->setText(QString::number(kg, 'f', 1));
        ui->kg_lb->setText("公斤");
    }
    else {
        // 1000000 g 以上 → 顯示「噸」
        double ton = global::carbonReduction()  / 1000000.0;
        ui->carbon_lb->setText(QString::number(ton, 'f', 1));
        ui->kg_lb->setText("噸");
    }
    ui->kg_lb->adjustSize();// 讓單位自適應長度

    double carbon_KG = global::carbonReduction() / 1000.0; // 克 → 公斤

    double days = carbon_KG / 60.0 * 365.0;                // 換算成天數

    qDebug()<<"day"<<days;

    if (days < 1.0) {
        // 小於 1 天 → 顯示小時
        int hours = qRound(days * 24.0);
        ui->tree_lb->setText(QString::number(hours));
        ui->date_lb->setText("小時");
    } else {
        // 大於等於 1 天 → 顯示天
        int roundedDays = qRound(days);
        ui->tree_lb->setText(QString::number(roundedDays));
        ui->date_lb->setText("天");
    }

    ui->date_lb->adjustSize(); // 自適應單位寬度

    startCountdown(60);
}

void nmb_endpage::hideEvent(QHideEvent *event)
{
    ui->carbon_lb->setText("0.0");
    ui->tree_lb->setText("0");
    countDownTimer->stop();
}

void nmb_endpage::mousePressEvent(QMouseEvent *event)
{
    startCountdown(60);
}

void nmb_endpage::startCountdown(int seconds)
{
    countdownValue = seconds;
    ui->countdown_lb->setText(QString::number(countdownValue));
    // 確保計時器不會重複連線
    disconnect(countDownTimer, nullptr, this, nullptr);
    connect(countDownTimer, &QTimer::timeout, this, &nmb_endpage::updateCountLabel);
    countDownTimer->start(1000);  // 每 1 秒觸發一次
}

void nmb_endpage::updateCountLabel()
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
        global::monitor()->checkStatus();//操作完成馬上檢測
    }
    else
    {
        ui->countdown_lb->setText(QString::number(countdownValue));
    }
}


void nmb_endpage::on_backhome_btn_clicked()
{
    QStackedWidget *stack = qobject_cast<QStackedWidget*>(parent());
    if (stack) {
        stack->setCurrentIndex(0);   // 切至首頁
    }
    global::monitor()->checkStatus();//操作完成馬上檢測
}

