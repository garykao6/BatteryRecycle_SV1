#ifndef NMB_ENDPAGE_H
#define NMB_ENDPAGE_H

#include <QWidget>
#include <QTimer>
namespace Ui {
class nmb_endpage;
}

class nmb_endpage : public QWidget
{
    Q_OBJECT

public:
    explicit nmb_endpage(QWidget *parent = nullptr);
    ~nmb_endpage();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;


private slots:
    void startCountdown(int seconds);
    void updateCountLabel();

    void on_backhome_btn_clicked();

private:
    Ui::nmb_endpage *ui;
    // QTimer countDownTimer;//倒計時時鐘
    QTimer *countDownTimer = nullptr;//倒計時時鐘
    int countdownValue = 60;
};

#endif // NMB_ENDPAGE_H
