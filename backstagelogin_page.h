#ifndef BACKSTAGELOGIN_PAGE_H
#define BACKSTAGELOGIN_PAGE_H

#include <QWidget>
#include <QPointer>
#include <QLineEdit>   // ★ 必須加這個

namespace Ui {
class backstageLogin_page;
}

class backstageLogin_page : public QWidget
{
    Q_OBJECT

public:
    explicit backstageLogin_page(QWidget *parent = nullptr);
    ~backstageLogin_page();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void setConnect();
    void onDigitClicked(int digitValue);
    void onLetterClicked(const QString &text);


    void on_symbol_1_lb_clicked();

    void on_symbol_2_lb_clicked();

    void on_delete_btn_clicked();

    void on_make_sure_btn_clicked();

    void on_close_btn_clicked();

    void on_switch_letter_lb_clicked();

private:
    Ui::backstageLogin_page *ui;
    QPointer<QLineEdit> m_currentEdit;

    bool isUpper = true;//大小寫判斷
    //快訴查找帳號密碼
    QHash<QString, QString> userMap = {
        //維護人員
        {"ecoco" , "1"},
        {"ecoco1", "a1b2c3"},
        {"ecoco2", "d4e5f6"},

        // {"EA01", "A1B2C3"},
        // {"EA02", "D4E5F6"},
        // {"EA03", "G7H8J9"},
        // {"EA04", "K1L2M3"},
        // {"EA05", "N4P5Q6"},
        // {"EA06", "R7S8T9"},
        // {"EA07", "U1V2W3"},
        // {"EA08", "X4Y5Z6"},
        // {"EA09", "A7B8C9"},
        // {"EA10", "D1E2F3"},
        // {"EA11", "G4H5J6"},
        // {"EA12", "K7L8M9"},
        // {"EA13", "N1P2Q3"},
        // {"EA14", "R4S5T6"},
        // {"EA15", "U7V8W9"},
        // {"EA16", "X1Y2Z3"},
        // {"EA17", "A4B5C6"},
        // {"EA18", "D7E8F9"},
        // {"EA19", "G1H2J3"},
        // {"EA20", "K4L5M6"},
    };
};

#endif // BACKSTAGELOGIN_PAGE_H
