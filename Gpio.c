#include "Gpio.h"
#include <pigpio.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>

int16_t Hardware_Init(void)
{    
    printf("Init GPIO...\n");	
    
    //GPIO接腳功能設定
    gpioSetMode(PIN_CONNECT_LED, PI_OUTPUT);          //LED CONNECT
    gpioSetMode(PIN_SW_LED, PI_OUTPUT);               //LED SW
    gpioSetMode(PIN_HEIGH_LP, PI_OUTPUT);             //雷射發射輸出控制 HEIGH
    gpioSetMode(PIN_POSITION_LP, PI_OUTPUT);          //雷射發射輸出控制 POSITION
    gpioSetMode(PIN_LEN_LP, PI_OUTPUT);               //雷射發射輸出控制 LEN    
    gpioSetMode(EN_PIN, PI_OUTPUT);                   //TMC2440 EN
    gpioSetMode(CS_PIN, PI_OUTPUT);                   //TMC2440 CS
    gpioSetMode(DIR_PIN, PI_OUTPUT);                  //TMC2440 DIR 
      
    gpioSetMode(PIN_POSITION_R, PI_INPUT);            //雷射接收 POSITION
    gpioSetMode(PIN_LEN_R, PI_INPUT);                 //雷射接收 LEN
    gpioSetMode(PIN_HEIGH_R, PI_INPUT);               //雷射接收 HEIGH    
    gpioSetMode(PIN_MENTAL_SENSOR, PI_INPUT);         //金屬感測接收

    gpioSetMode(PIN_LED_R, PI_OUTPUT);                //LED R
    gpioSetMode(PIN_LED_G, PI_OUTPUT);                //LED G
    gpioSetMode(PIN_LED_B, PI_OUTPUT);                //LED B

    gpioSetPullUpDown(PIN_POSITION_R ,PI_PUD_UP);      // 需要上拉
    gpioSetPullUpDown(PIN_LEN_R ,PI_PUD_UP);           // 需要上拉
    gpioSetPullUpDown(PIN_HEIGH_R ,PI_PUD_UP);         // 需要上拉
    gpioSetPullUpDown(PIN_MENTAL_SENSOR ,PI_PUD_UP);   // 需要上拉            

    gpioSetMode(PIN_HEIGH_LP2, PI_INPUT);             //PCB 短路錯誤 先將設定為輸入 避免衝突     


    //GPIO輸出設定
    gpioWrite(PIN_CONNECT_LED, 0);
    gpioWrite(PIN_SW_LED, 1);

    gpioWrite(PIN_LED_R, 1);             //0:亮 / 1:滅
    gpioWrite(PIN_LED_G, 1);
    gpioWrite(PIN_LED_B, 1);

    gpioWrite(EN_PIN, 1); // 預設關閉驅動器
    gpioWrite(CS_PIN, 1);   
    
    gpioWrite(PIN_POSITION_LP, 0);       //雷射關閉
    gpioWrite(PIN_LEN_LP, 0);        
    gpioWrite(PIN_HEIGH_LP, 0);  

    
    gpioWrite(DIR_PIN, 1);               //預設前進  

//    printf("SETUP PWM SPEED %d \n" , PWM_SPEED);	
    gpioSetMode(STEP_PIN, PI_OUTPUT);
    gpioPWM(STEP_PIN, PWM_DUTY);                         // duty=50%
    gpioSetPWMfrequency(STEP_PIN, PWM_SPEED);
                              
    return 0;
}

int16_t Hardware_Close(void)
{    
    printf("Close GPIO...\n");	
 
    //GPIO輸出設定
    gpioWrite(PIN_CONNECT_LED, 1);    
    gpioWrite(PIN_SW_LED, 1);    

    gpioWrite(EN_PIN, 1); // 預設關閉驅動器
    gpioWrite(CS_PIN, 1);   

    gpioWrite(PIN_HEIGH_LP, 0);         
    gpioWrite(PIN_POSITION_LP, 0);    
    gpioWrite(PIN_LEN_LP, 0);        

//    gpioSetMode(PIN_PWM1, PI_OUTPUT);
//    gpioWrite(PIN_PWM1, 0);    
//    gpioSetMode(PIN_PWM2, PI_OUTPUT);
//    gpioWrite(PIN_PWM2, 0);    
                              
    return 0;
}

void Led_Select(int index)
{
    // 先全部關掉
    gpioWrite(PIN_LED_R, 1);   // 0:亮 / 1:滅
    gpioWrite(PIN_LED_G, 1);
    gpioWrite(PIN_LED_B, 1);

    // 根據 index 選擇要亮的 LED
    switch (index)
    {
        case 0:

            break;
        case 1:
            gpioWrite(PIN_LED_R, 0);  // R 亮
            break;
        case 2:
            gpioWrite(PIN_LED_G, 0);  // G 亮
            break;
        case 3:
            gpioWrite(PIN_LED_B, 0);  // B 亮
            break;
        default:
            // 其他值全部關
            break;
    }
}
