#include "pointupload_dialog.h"
#include "ui_pointupload_dialog.h"

pointUpload_Dialog::pointUpload_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::pointUpload_Dialog)
{
    ui->setupUi(this);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框嵌入
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框ㄏ
    setGeometry(parentWidget()->rect());
}

pointUpload_Dialog::~pointUpload_Dialog()
{
    delete ui;
}
