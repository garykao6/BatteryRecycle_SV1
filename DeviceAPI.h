#ifndef DEVICE_API_H
#define DEVICE_API_H

#ifdef __cplusplus
extern "C" {
#endif

// ================== 馬達狀態定義 ==================
#define MOTOR_STOP    0
#define MOTOR_FOWARD  1
#define MOTOR_BACK    2

// ================== API 介面 ==================
                                        //雷射組別 1:入口 2:長度 3:高度
// 裝置開關控制

void DeviceAPI_Open(void);              //初始化GPIO   
int DeviceAPI_Run(void);                //打開裝置(可開始接收電池),回傳錯誤碼(0:正常; 1:SPI錯誤; 2:TMC2240錯誤; 4:雷射接收1故障; 5:雷射接收2故障; 6:雷射接收3故障                                        
void DeviceAPI_Run_Stop(void);          //關閉裝置(不可開始接收電池)
void DeviceAPI_Close(void);             //關閉裝置(停止接收電池,並關閉所有GPIO)  

// 馬達控制(開發測試用)
void DeviceAPI_MotorForward(void);      //馬達前進
void DeviceAPI_MotorBack(void);         //馬達後退 
void DeviceAPI_MotorStop(void);         //馬達停止

// 狀態查詢
int  DeviceAPI_GetMotorStatus(void);    //馬達狀態      0:停止; 1:前進; 2:後退; 3:異常         
int  DeviceAPI_GetBattery(int index);   //電池接收數    0:異物退出數量 ;1:3號電池 ;2:1號電池 ;3:4號電池 ;4:5號電池 ;5:6號電池; 6:2&9號電池 ;7:異常動作(釣魚)次數
int  DeviceAPI_GetSendor(int index);    //感測器及時狀態 0:位置感測 1:長度感測 2:高度感測 3:金屬感測
int  DeviceAPI_GetMotorStalled(void);   //馬達堵轉次數
int  DeviceAPI_GetMotorErrorCode(void); //馬達異常錯誤碼 0:正常; 11:過溫保護觸發; 12:過溫預警; 13:A相短路至GND; 14:B相短路至GND; 15:A相短路至VM; 16:B相短路至VM
                                        //                    7:雷射組1故障或阻塞; 8:雷射組2故障或阻塞; 9:雷射組3故障或阻塞;  10:金屬感測故障或阻塞;  
//LED控制
void LedAPI_Select(int index);          //index = 0 ,r亮; =1 g亮, =2 b亮

#ifdef __cplusplus
}
#endif

#endif // DEVICE_API_H
