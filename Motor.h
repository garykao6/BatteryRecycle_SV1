#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

#define STEPS_PER_REV 3200 // 每圈步數 (200步 × 16微步)


//TMC2240錯誤碼
#define TMC2240_ERROR_SPI 1
#define TMC2240_ERROR_REG 2

//設定除錯訊息是否輸出-馬達狀態
//#define _DEBUG_MOTOR

//// 設定馬達速度（PWM頻率）
//void Motor_SetSpeed(int pwm_freq);

// 打開馬達
void Motor_Run(void);

// 停止馬達
void Motor_Stop(void);

// 清理資源（關閉 SPI、停止 LED 執行緒等）
void Motor_Cleanup(void);

// TMC2240 初始化（SPI 寫寄存器設定）
int16_t Tmc2240_init(void);

// TMC2240 讀取電流相關資訊
uint32_t TMC2240_ReadCurrent(void);

void Motor_Foward(void);
void Motor_Back(void);
//void step_motor(int dir, int revs, int delay_us) ;

#endif // MOTOR_H
