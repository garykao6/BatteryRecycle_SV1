#include "mainwindow.h"

extern "C" {
#include "DeviceAPI.h"   // 只引入 API，不再直接 include Motor/Gpio/Proc_fw
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), gpio(5) // 使用 GPIO5
{
   toggleButton = new QPushButton("開啟裝置", this);
   toggleButton->setGeometry(30, 30, 80, 60);

   Button_R1 = new QPushButton("RUN", this);
   Button_R1->setGeometry(130, 30, 80, 60);

   connect(toggleButton, &QPushButton::clicked, this, &MainWindow::onToggleClicked);
   connect(Button_R1   , &QPushButton::clicked, this, &MainWindow::onRunClicked);    	

   label1 = new QLabel("馬達狀態:", this);
   label1->setGeometry(40, 100, 150, 40);

   label_sensor = new QLabel("感測器狀態", this);
   label_sensor->setGeometry(40, 130, 150, 40);    

   label2 = new QLabel("電池3&4號:", this);
   label2->setGeometry(40, 160, 150, 40); 
   label3 = new QLabel("電池9號:", this);
   label3->setGeometry(40, 190, 150, 40); 
   label4 = new QLabel("異物排除:", this);
   label4->setGeometry(40, 220, 150, 40); 


   timer1 = new QTimer(this);
   connect(timer1, &QTimer::timeout, this, [=]() mutable {
       label1->setText(QString("馬達狀態: %1").arg(DeviceAPI_GetMotorStatus()));
       label2->setText(QString("電池3&4號: %1").arg(DeviceAPI_GetBattery(1)));       
       label3->setText(QString("電池9號  : %1").arg(DeviceAPI_GetBattery(2)));              
       label4->setText(QString("異物排除 : %1").arg(DeviceAPI_GetBattery(0)));                     
       
       //label_sensor->setText(QString("感測器狀態 : %1").arg(DeviceAPI_GetSendor(0)));                     
		 label_sensor->setText(    QString("感測器狀態 : %1 , %2 , %3 , %4") .arg(DeviceAPI_GetSendor(0))
        																		             .arg(DeviceAPI_GetSendor(1))       
        																		             .arg(DeviceAPI_GetSendor(2))       
        																		             .arg(DeviceAPI_GetSendor(3)) );              																		                     																		             
   });
   timer1->start(100); // 每0.1秒更新一次
}

MainWindow::~MainWindow()  // 程式離開觸發點
{	
   DeviceAPI_Close();
}

void MainWindow::onToggleClicked() {
    if(ledOn) {
       ledOn = false;
       DeviceAPI_Close();     
    }
    else {
       ledOn = true;
       DeviceAPI_Open();
    }
    gpio.setState(ledOn);    
    toggleButton->setText(ledOn ? "關閉裝置" : "開啟裝置");    
}

void MainWindow::onRunClicked() {
    if(DeviceAPI_GetMotorStatus() != MOTOR_FOWARD) {
       printf("Motor_Foward\n");
       DeviceAPI_MotorForward();
    }
    else {
       printf("Motor_Back\n");
       DeviceAPI_MotorBack();
    }   
}
