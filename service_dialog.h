#ifndef SERVICE_DIALOG_H
#define SERVICE_DIALOG_H

#include <QDialog>

namespace Ui {
class service_Dialog;
}

class service_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit service_Dialog(QWidget *parent = nullptr);
    ~service_Dialog();

private slots:
    void on_close_btn_clicked();

private:
    Ui::service_Dialog *ui;
};

#endif // SERVICE_DIALOG_H
