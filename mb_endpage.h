#ifndef MB_ENDPAGE_H
#define MB_ENDPAGE_H

#include <QWidget>
#include <QTimer>
namespace Ui {
class mb_EndPage;
}

class mb_EndPage : public QWidget
{
    Q_OBJECT

public:
    explicit mb_EndPage(QWidget *parent = nullptr);
    ~mb_EndPage();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;


private slots:
    void startCountdown(int seconds);
    void updateCountLabel();
    void on_backhome_btn_clicked();


private:
    Ui::mb_EndPage *ui;
    QTimer *countDownTimer = nullptr;//倒計時時鐘
    int countdownValue = 60;
};

#endif // MB_ENDPAGE_H
