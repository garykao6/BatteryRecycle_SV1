#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include "playtool.h"

namespace Ui {
class HomePage;
}

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_mb_btn_clicked();

    void on_service_btn_clicked();

    void on_nmb_btn_clicked();

    void on_pushButton_clicked();

    void on_go_backstage_login_btn_clicked();

    void onstoreInfoAckReceived(const QJsonObject& obj);

signals:
    void backstage_login();

private:
    Ui::HomePage *ui;
    int clickCount = 0; // 點擊次數
    qint64 lastClickTime = 0;  // 記錄上一次點擊的時間
};

#endif // HOMEPAGE_H
