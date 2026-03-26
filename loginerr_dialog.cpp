#include "loginerr_dialog.h"
#include "ui_loginerr_dialog.h"

loginErr_Dialog::loginErr_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::loginErr_Dialog)
{
    ui->setupUi(this);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框ㄏ
    setGeometry(parentWidget()->rect());
}

loginErr_Dialog::~loginErr_Dialog()
{
    delete ui;
}
