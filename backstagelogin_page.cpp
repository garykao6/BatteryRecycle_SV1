#include "backstagelogin_page.h"
#include "ui_backstagelogin_page.h"
#include <QPushButton>
#include <QStackedWidget>
#include "global.h"
backstageLogin_page::backstageLogin_page(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::backstageLogin_page)
{
    ui->setupUi(this);

    setConnect();
    connect(qApp, &QApplication::focusChanged, this,
            [this](QWidget*, QWidget* now){
                if (auto* e = qobject_cast<QLineEdit*>(now))
                    m_currentEdit = e;
            });
}

backstageLogin_page::~backstageLogin_page()
{
    delete ui;
}
void backstageLogin_page::showEvent(QShowEvent *event)
{
    m_currentEdit = ui->Account_edit;//先指定要輸入的物件 避免空指標程式崩潰
    ui->Account_edit->setText("");
    ui->Password_edit->setText("");
}

void backstageLogin_page::hideEvent(QHideEvent *event)
{

}

void backstageLogin_page::setConnect()
{
    // 數字鍵 0~9
    for (int i = 0; i <= 9; i++) {
        QString name = QString("number_btn_%1").arg(i);   // 和你的物件名一致
        QPushButton *btn = findChild<QPushButton*>(name);
        if (btn) {
            connect(btn, &QPushButton::clicked, this, [this, i]{
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

void backstageLogin_page:: onDigitClicked(int digitValue)
{
    m_currentEdit->insert(QString::number(digitValue));

}

void backstageLogin_page::onLetterClicked(const QString &text)
{
    m_currentEdit->insert(text);
}

void backstageLogin_page::on_switch_letter_lb_clicked()
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

void backstageLogin_page::on_symbol_1_lb_clicked()
{
    m_currentEdit->insert(ui->symbol_1_lb->text());
}


void backstageLogin_page::on_symbol_2_lb_clicked()
{
    m_currentEdit->insert(ui->symbol_2_lb->text());
}


void backstageLogin_page::on_delete_btn_clicked()
{
    m_currentEdit->backspace();
}


void backstageLogin_page::on_make_sure_btn_clicked()
{
    QString inputUser = ui->Account_edit->text();
    QString inputPass = ui->Password_edit->text();

    if (userMap.contains(inputUser) && userMap.value(inputUser) == inputPass) {
        ui->remid_lb->setText("");
        qDebug() << "✅ 登入成功:" << inputUser;
        global::op_id = inputUser;
        QStackedWidget *stack = qobject_cast<QStackedWidget*>(this->parentWidget());
        if (stack) {
            stack->setCurrentIndex(9); //進入工程模式
        }
    } else {
        qDebug() << "❌ 帳號或密碼錯誤";
        ui->remid_lb->setText("帳號或密碼錯誤");
    }
}


void backstageLogin_page::on_close_btn_clicked()
{
    QStackedWidget *stack = qobject_cast<QStackedWidget*>(this->parentWidget());
    if (stack) {
        stack->setCurrentIndex(0); //回首頁
        global::getVideoShow()->show();//輪播顯示
    }
}




