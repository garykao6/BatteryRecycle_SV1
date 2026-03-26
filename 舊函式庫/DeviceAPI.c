
#include <stdint.h>  
#include <stdbool.h>
#include "DeviceAPI.h"
#include "Motor.h"
#include "Gpio.h"
#include "Proc_fw.h"
#include "ws281x.h"


int DeviceAPI_Open(void) {	
    return Device_Run();   // 原本初始化裝置
}

void DeviceAPI_Close(void) {
    Device_Close();        // 原本關閉裝置
}

int DeviceAPI_GetMotorErrorCode(void){
	
    return GetMotorErrorCode(); 
}


void DeviceAPI_MotorForward(void) {
    devicd_struct.flag_dir = true;
    devicd_struct.status_motor = M_FOWARD;
}

void DeviceAPI_MotorBack(void) {
    devicd_struct.flag_dir = false;
    devicd_struct.status_motor = M_BACK;
}

void DeviceAPI_MotorStop(void) {
    devicd_struct.status_motor = M_STOP;
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


int LedAPI_Run(void) {
   return Led_Run();
   
}

void LedAPI_Select(int index) {
	
   ledx33_mode = index;
   
}

void LedAPI_Set(uint32_t color) {
	
   led_color_set(color);
}

void LedAPI_Close(void) {

    Led_close();              //需要先關閉LED 再關閉Device
    
}

void Relay1API_On(void)
{
   RelayNo1_On();	
}

void Relay1API_Off(void)
{
   RelayNo1_Off();	
}

void Relay2API_On(void)
{
   RelayNo2_On();	
}

void Relay2API_Off(void)
{
   RelayNo2_Off();	
}
