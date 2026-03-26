#ifndef MACHINEERR_DIALOG_H
#define MACHINEERR_DIALOG_H

#include <QDialog>

namespace Ui {
class machineErr_Dialog;
}

class machineErr_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit machineErr_Dialog(QWidget *parent = nullptr);
    ~machineErr_Dialog();
    void setText(const QString& str);

private:
    Ui::machineErr_Dialog *ui;
};

#endif // MACHINEERR_DIALOG_H
