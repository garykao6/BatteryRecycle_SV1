#ifndef MACHINERECORD_DIALOG_H
#define MACHINERECORD_DIALOG_H

#include <QDialog>

namespace Ui {
class machineRecord_Dialog;
}

class machineRecord_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit machineRecord_Dialog(QWidget *parent = nullptr);
    ~machineRecord_Dialog();

private:
    Ui::machineRecord_Dialog *ui;
};

#endif // MACHINERECORD_DIALOG_H
