#include "erroritem_dialog.h"
#include "ui_erroritem_dialog.h"
#include "clockbus.h"
#include "global.h"
#include "mqtthelper.h"
// ErrorItem_Dialog::ErrorItem_Dialog(SYSTEM_ERROR_ITEM errItem,QWidget *parent)
ErrorItem_Dialog::ErrorItem_Dialog(QWidget *parent,SYSTEM_ERROR_ITEM errItem)
    : QDialog(parent)
    , ui(new Ui::ErrorItem_Dialog)
{
    ui->setupUi(this);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);  // 無邊框嵌入
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框 最上層

    // 異常畫面（oosImg / nightBreak）顯示時間與站點，對齊首頁
    ui->time_lb->setText(ClockBus::instance()->nowText());
    ui->station_lb->setText(global::storeInfo);
    connect(ClockBus::instance(), &ClockBus::minuteTick, this, [this](const QDateTime&, const QString& s) {
        ui->time_lb->setText(s);
    });
    connect(&global::mqtt(), &MqttHelper::storeInfoAckReceived, this, [this](const QJsonObject& obj) {
        const QJsonObject storeObj = obj.value("store").toObject();
        const QString storeName = storeObj.value("name").toString();
        if (!storeName.isEmpty()) {
            global::storeInfo = storeName;
            ui->station_lb->setText(global::storeInfo);
        }
    });
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
    ui->time_lb->setVisible(false);
    ui->station_lb->setVisible(false);
    ui->station_lb_2->setVisible(false);
}

void ErrorItem_Dialog::showMaintain()
{
    ui->show_lb->setStyleSheet(R"(
        QLabel {
            image: url(:/IMAGES/UI_IMAGE/system_err_image/oosImg.png);
            border: none;
        }
    )");
    ui->time_lb->setVisible(true);
    ui->station_lb->setVisible(true);
    ui->station_lb_2->setVisible(true);
}

void ErrorItem_Dialog::showFull()
{
    // ui->show_lb->setStyleSheet(R"(
    //     QLabel {
    //         image: url(:/IMAGES/UI_IMAGE/system_error_image/boxFullWarning.png);
    //         border: none;
    //     }
    // )");
    ui->time_lb->setVisible(false);
    ui->station_lb->setVisible(false);
    ui->station_lb_2->setVisible(false);
}

void ErrorItem_Dialog::showNetwork()
{
    // ui->show_lb->setStyleSheet(R"(
    //     QLabel {
    //         image: url(:/IMAGES/UI_IMAGE/system_error_image/network_disconnect.png);
    //         border: none;
    //     }
    // )");
    ui->time_lb->setVisible(false);
    ui->station_lb->setVisible(false);
    ui->station_lb_2->setVisible(false);
}

void ErrorItem_Dialog::showIdentify()
{
    ui->show_lb->setStyleSheet(R"(
        QLabel {
            image: url(:/IMAGES/UI_IMAGE/system_err_image/rejectWarning.png);
            border: none;
        }
    )");
    ui->time_lb->setVisible(false);
    ui->station_lb->setVisible(false);
    ui->station_lb_2->setVisible(false);
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
    ui->time_lb->setVisible(true);
    ui->station_lb->setVisible(true);
    ui->station_lb_2->setVisible(true);
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
