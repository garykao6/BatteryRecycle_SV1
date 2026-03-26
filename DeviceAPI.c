
#include <stdint.h>  
#include <stdbool.h>
#include "DeviceAPI.h"
#include "Motor.h"
#include "Gpio.h"
#include "Proc_fw.h"
//#include "ws281x.h"

void DeviceAPI_Open(void) {
     Device_Open();        // 初始化裝置
}

void DeviceAPI_Close(void) {
    Device_Close();        // 原本關閉裝置
}

int DeviceAPI_Run(void) {
    return Device_Run();   // 馬達功能啟動
}

void DeviceAPI_Run_Stop(void) {
    Device_Stop();        // 馬達功能停止
}


int DeviceAPI_GetMotorErrorCode(void){

    return GetMotorErrorCode();
}


void DeviceAPI_MotorForward(void) {
    devicd_struct.flag_dir = true;
    devicd_struct.status_motor = M_FOWARD;

    Motor_Foward();
    Motor_Run();                //馬達啟動
}

void DeviceAPI_MotorBack(void) {
    devicd_struct.flag_dir = false;
    devicd_struct.status_motor = M_BACK;

    Motor_Back();
    Motor_Run();                //馬達啟動
}

void DeviceAPI_MotorStop(void) {
    devicd_struct.status_motor = M_STOP;

    Motor_Stop();               //馬達停止
}

int DeviceAPI_GetMotorStatus(void) {
    return devicd_struct.status_motor;
}

int DeviceAPI_GetBattery(int index) {
    if (index < 0 || index >= 8) return -1;
    return devicd_struct.battery_count[index];
}

int DeviceAPI_GetSendor(int index) {
    if (index < 0 || index >= 4) return -1;
    return devicd_struct.sensor[index];
}

int DeviceAPI_GetMotorStalled(void) {
    return devicd_struct.stalled_cnt;
}


void LedAPI_Select(int index)
{

   Led_Select(index);

}



