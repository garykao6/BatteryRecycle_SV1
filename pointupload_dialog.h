#ifndef POINTUPLOAD_DIALOG_H
#define POINTUPLOAD_DIALOG_H

#include <QDialog>

namespace Ui {
class pointUpload_Dialog;
}

class pointUpload_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit pointUpload_Dialog(QWidget *parent = nullptr);
    ~pointUpload_Dialog();

private:
    Ui::pointUpload_Dialog *ui;
};

#endif // POINTUPLOAD_DIALOG_H
