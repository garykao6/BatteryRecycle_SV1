#ifndef MB_LOGINPAGE_H
#define MB_LOGINPAGE_H

#include <QWidget>
#include <QTimer>
namespace Ui {
class mb_LoginPage;
}

class mb_LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit mb_LoginPage(QWidget *parent = nullptr);
    ~mb_LoginPage();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void setConnect();

    void onDigitClicked(int digitValue);
    void on_clear_btn_clicked();

    void setErrorText();
    void setOriginalText();

    void startCountdown(int seconds);
    void updateCountLabel();

    void on_back_btn_clicked();

    void on_makesure_btn_clicked();
    void onLoginAckReceived(const QJsonObject &payload);

private:
    Ui::mb_LoginPage *ui;
    // QTimer inputTimer;  //輸入時間偵測
    QTimer *countDownTimer = nullptr; //倒計時時鐘
    int countdownValue = 30;
    bool login_is = false;

};

#endif // MB_LOGINPAGE_H
