#ifndef BACKSTAGE_H
#define BACKSTAGE_H

#include <QWidget>
#include <QStackedWidget>
#include "global.h"
#include "DeviceAPI.h"
#include <QProcess>
#include "setting_dialog.h"
#include <QTimer>
namespace Ui {
class backstage;
}

class backstage : public QWidget
{
    Q_OBJECT

public:
    explicit backstage(QWidget *parent = nullptr);
    ~backstage();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void function_Hidden();//關閉功能及隱藏

private slots:
    void on_home_btn_clicked();

    void on_stop_service_btn_clicked();

    void on_system_down_btn_clicked();

    void on_system_reboot_btn_clicked();

    void on_Forward_btn_clicked();

    void on_Reverse_btn_clicked();

    void on_stop_btn_clicked();

    void on_off_btn_clicked();

    void on_red_btn_clicked();

    void on_blue_btn_clicked();

    void on_green_btn_clicked();

    void on_clear_btn_clicked();

    void on_setting_btn_clicked();

    void changeStatusLight();

    void showSystemOffTIme();

    void on_clear_carbon_btn_clicked();

signals:
    void idleWatchPoke();

private:
    Ui::backstage *ui;
    QTimer *timer = nullptr;
    int m_motorstatus;

    bool bit0LeftMost = false; // false = bit0 在最右邊 (LSB)
    inline int idxForBit(int b) const { return bit0LeftMost ? b : (47 - b); }
};

#endif // BACKSTAGE_H
