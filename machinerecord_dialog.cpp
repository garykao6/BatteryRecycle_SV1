#include "machinerecord_dialog.h"
#include "ui_machinerecord_dialog.h"

machineRecord_Dialog::machineRecord_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::machineRecord_Dialog)
{
    ui->setupUi(this);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框嵌入
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框
    setGeometry(parentWidget()->rect());
}

machineRecord_Dialog::~machineRecord_Dialog()
{
    delete ui;
}
