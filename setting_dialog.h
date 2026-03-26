#ifndef SETTING_DIALOG_H
#define SETTING_DIALOG_H

#include <QDialog>
#include "global.h"
#include <QLineEdit>   // ★ 必須加這個
#include <QRegularExpressionValidator>
#include <QMessageBox>

namespace Ui {
class setting_Dialog;
}

class setting_Dialog : public QDialog
{
    Q_OBJECT



public:
    explicit setting_Dialog(QWidget *parent = nullptr);
    ~setting_Dialog();
    enum DialogCode
    {
        SET_POWEROFF = 100,
    };

private slots:
    void on_make_sure_btn_clicked();

    void on_close_btn_clicked();

    void setConnect();

    void onDigitClicked(int digitValue);
    void onLetterClicked(const QString &text);

    void on_delete_btn_clicked();

    void on_switch_letter_lb_clicked();

    void on_symbol_1_lb_clicked();

    void on_symbol_2_lb_clicked();

    void on_time_make_sure_btn_clicked();

    void on_power_btn_clicked();

private:
    Ui::setting_Dialog *ui;
    QPointer<QLineEdit> m_currentEdit;
    bool isUpper = true;//大小寫判斷
};

#endif // SETTING_DIALOG_H
