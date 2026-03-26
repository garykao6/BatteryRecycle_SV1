#include "service_dialog.h"
#include "ui_service_dialog.h"
#include "global.h"
service_Dialog::service_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::service_Dialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框嵌入
    // ui->close_logo_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框
    setGeometry(parentWidget()->rect());

    //關閉按鈕上面的label接收滑鼠事件避免按鈕無法點擊
    ui->close_logo_lb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

service_Dialog::~service_Dialog()
{
    delete ui;
}

void service_Dialog::on_close_btn_clicked()
{
    global::playSound("sounds/返回.mp3");
    accept();
}

