#ifndef WELCOM_DIALOG_H
#define WELCOM_DIALOG_H

#include <QDialog>

namespace Ui {
class welcom_Dialog;
}

class welcom_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit welcom_Dialog(QWidget *parent = nullptr);
    ~welcom_Dialog();

private:
    Ui::welcom_Dialog *ui;
};

#endif // WELCOM_DIALOG_H
