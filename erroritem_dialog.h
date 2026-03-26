#ifndef ERRORITEM_DIALOG_H
#define ERRORITEM_DIALOG_H

#include <QDialog>
#include <QStackedWidget>
#include <QDateTime>

// 將裸列舉改成 enum class，避免命名衝突
// enum class SYSTEM_ERROR_ITEM {
//     ERROR_MACHINE,
//     ERROR_FULL,
//     ERROR_MAINTAIN,
//     ERROR_NETWORK,
//     ERROR_IDENTIFY,
//     NORMAL
// };
// Q_ENUM(SYSTEM_ERROR_ITEM)


enum SYSTEM_ERROR_ITEM {
    ERROR_MACHINE,
    ERROR_FULL,
    ERROR_MAINTAIN,
    ERROR_NETWORK,
    ERROR_IDENTIFY,
    NORMAL
};

namespace Ui {
class ErrorItem_Dialog;
}


class ErrorItem_Dialog : public QDialog
{
    Q_OBJECT


public:
    // explicit ErrorItem_Dialog(SYSTEM_ERROR_ITEM errItem = SYSTEM_ERROR_ITEM::NORMAL, QWidget *parent = nullptr);
    explicit ErrorItem_Dialog(QWidget *parent = nullptr, SYSTEM_ERROR_ITEM errItem = SYSTEM_ERROR_ITEM::NORMAL);
    ~ErrorItem_Dialog();

    void showMachine();
    void showMaintain();//維修中
    void showFull();
    void showNetwork();
    void showIdentify();//無法辨識
    void showSleep();//休眠


private slots:
    void on_go_backstage_login_btn_clicked();

private:
    Ui::ErrorItem_Dialog *ui;
    int clickCount = 0; // 點擊次數
    qint64 lastClickTime = 0;  // 記錄上一次點擊的時間
};

#endif // ERRORITEM_DIALOG_H
