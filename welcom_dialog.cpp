#include "welcom_dialog.h"
#include "ui_welcom_dialog.h"

welcom_Dialog::welcom_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::welcom_Dialog)
{
    ui->setupUi(this);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框嵌入
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框ㄏ
    setGeometry(parentWidget()->rect());
}

welcom_Dialog::~welcom_Dialog()
{
    delete ui;
}
