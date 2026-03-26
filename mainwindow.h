#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include "gpio_interface.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onToggleClicked();
    void onRunClicked();    


private:
    QPushButton *toggleButton;
    QPushButton *Button_R1;  
    QLabel      *label1;
    QLabel      *label2;    
    QLabel      *label3;        
    QLabel      *label4;            
    QLabel      *label_sensor;                
    QTimer      *timer1;   
    GPIOInterface gpio;
    bool ledOn = false;
};

#endif
