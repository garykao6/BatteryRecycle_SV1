#ifndef MB_PUTINBATTERYPAGE_H
#define MB_PUTINBATTERYPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include "gpio_interface.h"
#include <QMediaPlayer>
#include <QVideoWidget>
#include "erroritem_dialog.h"
namespace Ui {
class mb_PutINBatteryPage;
}

class mb_PutINBatteryPage : public QWidget
{
    Q_OBJECT

public:
    explicit mb_PutINBatteryPage(QWidget *parent = nullptr);
    ~mb_PutINBatteryPage();

signals:
    void idleWatchPoke(); //重新計算待機訊號


protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void startCountdown(int seconds);
    void updateCountLabel();
    void on_makesure_btn_clicked();
    QMap<QString,int> collectBatteryStatus() const;

private:
    Ui::mb_PutINBatteryPage *ui;
    // QTimer countDownTimer;//倒計時時鐘
    QTimer *countDownTimer = nullptr; //倒計時時鐘
    int countdownValue = 60;

    QLabel      *label_sensor;
    QTimer      *timer1;
    GPIOInterface gpio;
    bool ledOn = false;

    int motorStatus = 0; //馬達狀態

    int point10_count  = 0;
    int point5_count   = 0;

    int battery1    = 0; //1號電池數
    int battery29    = 0; //2&9號電池數
    int battery3   = 0; //3號電池數
    int battery4    = 0; //4號電池數
    int battery5    = 0; //5號電池數
    int battery6    = 0; //6號電池數
    int err_op_count = 0;// 釣魚次數 3次直接登出
    int obstacle    = 0;//異物數量

    int sensor0     = 0;  //位置感測值
    int sensor1     = 0;  //長度感測值
    int sensor2     = 0;  //高度感測值
    int sensor3     = 0;  //金屬感測值


    int lastMotorStatus = 0;
    int lastpoint10_count   = 0;
    int lastpoint5_count    = 0;

    int lastbattery1    = 0; //1號電池數
    int lastbattery29    = 0; //2&9號電池數
    int lastbattery3   = 0; //3號電池數
    int lastbattery4    = 0; //4號電池數
    int lastbattery5    = 0; //5號電池數
    int lastbattery6    = 0; //6號電池數
    int last_err_op_count = 0;// 釣魚次數 3次直接登出
    int lastObstacle    = 0; //異物從0開始


    int lastSensor0     = 0;
    int lastSensor1     = 0;
    int lastSensor2     = 0;
    int lastSensor3     = 0;

    QMediaPlayer *video_player = nullptr;
    QVideoWidget *video_output = nullptr;
    ErrorItem_Dialog *err_dlg = nullptr;



};

#endif // MB_PUTINBATTERYPAGE_H
