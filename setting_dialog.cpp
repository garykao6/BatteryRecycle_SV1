#include "setting_dialog.h"
#include "ui_setting_dialog.h"
#include "global.h"
setting_Dialog::setting_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::setting_Dialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);  // 無邊框 最上層

    setConnect();//連接按鈕

    m_currentEdit = ui->inpute_edit;

    connect(qApp, &QApplication::focusChanged, this,
            [this](QWidget*, QWidget* now){
                if (auto* e = qobject_cast<QLineEdit*>(now))
                    m_currentEdit = e;
            });

    connect(&global::mqtt(), &MqttHelper::poweronDateAckReceived,this,[this](QJsonObject payload)
            {
                // 1. 取得內層的 poweronDateAck 物件
                QJsonObject ack = payload["poweronDateAck"].toObject();

                // 2. 提取 code 與 note
                QString code = ack["code"].toString();
                QString note = ack["note"].toString();

                // 3. 根據內容設定 UI 顯示文字
                if (code == "OK") {
                    ui->power_message_lb->setStyleSheet(
                        "font: 700 30px \"Noto Sans CJK TC\";"
                        "color: green;"
                        "image: none;"
                        "background-color: transparent;"
                        );
                    ui->power_message_lb->setText("設定成功");
                } else {
                    // 失敗顯示紅色
                    ui->power_message_lb->setStyleSheet(
                        "font: 700 30px \"Noto Sans CJK TC\";"
                        "color: red;"
                        "image: none;"
                        "background-color: transparent;"
                        );
                    // 如果 note 是空的，顯示預設錯誤訊息
                    QString errorMsg = note.isEmpty() ? "設定失敗 (未知錯誤)" : note;
                    ui->power_message_lb->setText(errorMsg);
                }
            });

    // ui->inpute_edit->setText(global::readConfigValue("deviceId","0000").toString());
    ui->time_edit->setText(global::readConfigValue("powerOffTime","").toString());
}

setting_Dialog::~setting_Dialog()
{
    delete ui;
}

void setting_Dialog::on_make_sure_btn_clicked()
{
    global::deviceId = "FLCBRM03_"+ui->inpute_edit->text();
    global::mqtt().m_deviceId = "FLCBRM03_"+ui->inpute_edit->text();
    global::writeConfig("deviceId","FLCBRM03_"+ui->inpute_edit->text());//寫入檔案
    accept();
}

void setting_Dialog::on_close_btn_clicked()
{
    close();
}


void setting_Dialog::setConnect()
{
    // 數字鍵 0~9
    for (int i = 0; i <= 9; i++) {
        QString name = QString("number_btn_%1").arg(i);   // 和你的物件名一致
        QPushButton *btn = findChild<QPushButton*>(name);
        if (btn) {
            connect(btn, &QPushButton::clicked, this, [this, i] {
                onDigitClicked(i);
            });
        }
    }

    // 字母鍵 A~Z
    for (char c = 'A'; c <= 'Z'; ++c) {
        QString name = QString("letter_btn_%1").arg(QChar(c));
        if (auto btn = findChild<QPushButton*>(name)) {
            connect(btn, &QPushButton::clicked, this, [this, btn] {
                onLetterClicked(btn->text());   // ✅ 用按鈕顯示文字
            });
        }
    }
}

void setting_Dialog:: onDigitClicked(int digitValue)
{
    // ui->inpute_edit->insert(QString::number(digitValue));
    // if (ui->inpute_edit->text().size() >= 10) {
    //     // QApplication::beep();                    // 超過就提示
    //     // 可選：若有 statusbar
    //     // ui->statusbar->showMessage("最多 10 碼", 1500);
    //     return;
    // }

    // if( ui->inpute_edit->text().size()<10)//10碼限制
    // {
    //     ui->inpute_edit->insert(QString::number(digitValue));
    // }

    m_currentEdit->insert(QString::number(digitValue));
}

void setting_Dialog::onLetterClicked(const QString &text)
{
    // if (ui->inpute_edit->text().size() < 10)
    //     ui->inpute_edit->insert(text);
    m_currentEdit->insert(text);
}

void setting_Dialog::on_delete_btn_clicked()
{
    // ui->inpute_edit->backspace();
    m_currentEdit->backspace();
}


void setting_Dialog::on_switch_letter_lb_clicked()
{

    for (char c = 'A'; c <= 'Z'; ++c) {
        QString name = QString("letter_btn_%1").arg(QChar(c));
        if (auto btn = findChild<QPushButton*>(name)) {
            QString current = btn->text();
            btn->setText(isUpper ? current.toUpper() : current.toLower());
        }
    }

    // 同步更新這個按鈕本身的顯示
    ui->switch_letter_lb->setText(isUpper ? "小寫" : "大寫");
    isUpper = !isUpper;  // 切換狀態
}



void setting_Dialog::on_symbol_1_lb_clicked()
{
    // ui->inpute_edit->insert(ui->symbol_1_lb->text());
    m_currentEdit->insert(ui->symbol_1_lb->text());

}


void setting_Dialog::on_symbol_2_lb_clicked()
{
   // ui->inpute_edit->insert(ui->symbol_2_lb->text());
   m_currentEdit->insert(ui->symbol_2_lb->text());
}


void setting_Dialog::on_time_make_sure_btn_clicked()
{
    QString text = ui->time_edit->text();
    text.remove(':');
    // text.remove(QRegularExpression(R"([^0-9])"));  // 僅保留數字

    if (text.size() < 3) {
        QMessageBox::warning(this, "格式錯誤", "請至少輸入三位數（如 700 → 07:00）");
        return;
    }

    // 取前兩位當小時、後兩位當分鐘
    if (text.size() > 4)
        text = text.left(4);

    bool ok;
    int val = text.toInt(&ok);
    if (!ok) {
        // QMessageBox::warning(this, "錯誤", "請輸入有效數字");
        QMessageBox::warning(this, "回應", "將不使用自動關機");
        global::writeConfig("powerOffTime",ui->time_edit->text());//寫入檔案
        done(SET_POWEROFF); //QDialog 回傳碼(自己新增的)
        return;
    }

    int h = val / 100;
    int m = val % 100;

    if (h < 0 || h > 23 || m < 0 || m > 59) {
        QMessageBox::warning(this, "時間無效", "請輸入 0000~2359 的時間");
        return;
    }

    QString formatted = QString("%1:%2")
                            .arg(h, 2, 10, QLatin1Char('0'))
                            .arg(m, 2, 10, QLatin1Char('0'));
    ui->time_edit->setText(formatted);

    // 這裡可以接下來做你的邏輯，例如儲存時間
    qDebug() << "時間確認：" << formatted;
    global::writeConfig("powerOffTime",ui->time_edit->text());//寫入檔案
    done(SET_POWEROFF); //QDialog 回傳碼(自己新增的)
}

//合約開機
void setting_Dialog::on_power_btn_clicked()
{
    global::mqtt().sendpoweronDate();
}

