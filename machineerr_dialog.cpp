#include "machineerr_dialog.h"
#include "ui_machineerr_dialog.h"

machineErr_Dialog::machineErr_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::machineErr_Dialog)
{
    ui->setupUi(this);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框
    setGeometry(parentWidget()->rect());
}

machineErr_Dialog::~machineErr_Dialog()
{
    delete ui;
}

void machineErr_Dialog::setText(const QString& str)
{
    ui->label_2->setText(str);
}
