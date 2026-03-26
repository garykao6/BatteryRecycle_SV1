#include "erroritem_dialog.h"
#include "ui_erroritem_dialog.h"
// ErrorItem_Dialog::ErrorItem_Dialog(SYSTEM_ERROR_ITEM errItem,QWidget *parent)
ErrorItem_Dialog::ErrorItem_Dialog(QWidget *parent,SYSTEM_ERROR_ITEM errItem)
    : QDialog(parent)
    , ui(new Ui::ErrorItem_Dialog)
{
    ui->setupUi(this);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框嵌入
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框 最上層
}

ErrorItem_Dialog::~ErrorItem_Dialog()
{
    delete ui;
}


void ErrorItem_Dialog::showMachine()
{
    // ui->show_lb->setStyleSheet(R"(
    //     QLabel {
    //         image: url(:/IMAGES/UI_IMAGE/system_error_image/alarmWarning.png);
    //         border: none;
    //     }
    // )");
}

void ErrorItem_Dialog::showMaintain()
{
    ui->show_lb->setStyleSheet(R"(
        QLabel {
            image: url(:/IMAGES/UI_IMAGE/system_err_image/oosImg.png);
            border: none;
        }
    )");
}

void ErrorItem_Dialog::showFull()
{
    // ui->show_lb->setStyleSheet(R"(
    //     QLabel {
    //         image: url(:/IMAGES/UI_IMAGE/system_error_image/boxFullWarning.png);
    //         border: none;
    //     }
    // )");
}

void ErrorItem_Dialog::showNetwork()
{
    // ui->show_lb->setStyleSheet(R"(
    //     QLabel {
    //         image: url(:/IMAGES/UI_IMAGE/system_error_image/network_disconnect.png);
    //         border: none;
    //     }
    // )");
}

void ErrorItem_Dialog::showIdentify()
{
    ui->show_lb->setStyleSheet(R"(
        QLabel {
            image: url(:/IMAGES/UI_IMAGE/system_err_image/rejectWarning.png);
            border: none;
        }
    )");
}

//休眠
void ErrorItem_Dialog::showSleep()
{
    ui->show_lb->setStyleSheet(R"(
        QLabel {
            image: url(:/IMAGES/UI_IMAGE/system_err_image/nightBreak.png);
            border: none;
        }
    )");
}

void ErrorItem_Dialog::on_go_backstage_login_btn_clicked()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastClickTime > 3000) clickCount = 0;
    lastClickTime = now;
    ++clickCount;

    qDebug() << "點擊次數:" << clickCount;

    if (clickCount < 7) return;
    clickCount = 0;

    // ---- 這裡開始切頁 ----
    QStackedWidget* stack = qobject_cast<QStackedWidget*>(this->parentWidget());                // 用成員，不用 parentWidget()
    if (!stack) { qWarning() << "❌ 找不到父 stack（成員為空）"; return; }

    constexpr int kBackIndex = 8; //8 工程頁面
    const int n = stack->count();
    qDebug() << "stack =" << stack << "count =" << n << "target =" << kBackIndex;

    if (kBackIndex < 0 || kBackIndex >= n) {
        qWarning() << "❌ 目標索引不存在：" << kBackIndex << "(count =" << n << ")";
        return;
    }

    stack->setCurrentIndex(kBackIndex);                  // 用索引最安全
    qDebug() << "✅ 已切到 index" << kBackIndex;
}

