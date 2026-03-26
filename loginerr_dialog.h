#ifndef LOGINERR_DIALOG_H
#define LOGINERR_DIALOG_H

#include <QDialog>

namespace Ui {
class loginErr_Dialog;
}

class loginErr_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit loginErr_Dialog(QWidget *parent = nullptr);
    ~loginErr_Dialog();

private:
    Ui::loginErr_Dialog *ui;
};

#endif // LOGINERR_DIALOG_H
