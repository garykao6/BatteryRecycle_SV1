#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

/* Private define ------------------------------------------------------------*/
#define STEP_PIN    20                                                       // BCM GPIO18，支援硬體 ; PWM -> 20(未支援)
#define DIR_PIN     21                                                       // BCM GPIO23 -> 21
#define EN_PIN      16                                                       // BCM GPIO24 -> 16 
#define CS_PIN      8	                                                     // BCM GPIO25 -> 8 
#define PIN_CONNECT_LED  5                                                   // BCM GPIO5
#define PIN_SW_LED       25                                                  // BCM GPIO25
#define PIN_PWM1         18                                                  // BCM GPIO19
#define PIN_PWM2         19                                                  // BCM GPIO18

#define PIN_POSITION_LP  27                                                  // BCM GPIO17
#define PIN_LEN_LP       17                                                  // BCM GPIO27
#define PIN_HEIGH_LP     22                                                  // BCM GPIO22
#define PIN_HEIGH_LP2    4                                                   // BCM GPIO4
#define PIN_LEN_R        12                                                   //
#define PIN_POSITION_R   13                                                  //
#define PIN_HEIGH_R      6                                                  //
#define PIN_MENTAL_SENSOR 26                                                 // BCM GPIO26
#define PIN_LED_R  23
#define PIN_LED_G  18
#define PIN_LED_B  24

#define MENTAL_STATUS gpioRead(PIN_MENTAL_SENSOR)
#define LEN_STATUS    gpioRead(PIN_LEN_R)                                    //雷射接收器,沒接收到雷射=0 ,接收到雷射=1  
#define POS_STATUS    gpioRead(PIN_POSITION_R)
#define HEIGH_STATUS  gpioRead(PIN_HEIGH_R)

#define SPI_SPEED   1000000
#define PWM_SPEED   4000
#define PWM_DUTY    127            // duty=50%

extern int16_t Hardware_Init(void);
extern int16_t Hardware_Close(void);
extern void Led_Select(int index);

#endif // GPIO_H
