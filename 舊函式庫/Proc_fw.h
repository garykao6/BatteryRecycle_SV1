#ifndef PROC_FW_H
#define PROC_FW_H

#include <stdint.h>

//接收電池版本數量
#define BATTERY_TYPE_MAX 8              //0:異物退出數量; 1:3號電池; 2:1號電池; 3:4號電池; 4:5號電池; 5:6號電池; 6:2&9號電池; 7:釣魚&拉桿動作;
#define SENSOR_MAX       4              //4個感測器輸入
#define PULSE_WIDTH 3                   /*PULSE_WIDTH * 10ms =輸入寬度 */

//BATTERY_TYPE 電池種類
#define BT_OBJ           0              //異物 
#define BT_NUM_3         1              //3號電池
#define BT_NUM_1         2              //1號電池
#define BT_NUM_4         3              //4號電池
#define BT_NUM_5         4              //5號電池
#define BT_NUM_6         5              //6號電池
#define BT_NUM_2         6              //2&9號電池
#define BT_ERR           7              //釣魚動作

//status_motor 馬達狀態定義
#define M_STOP           0              //馬達停止 
#define M_FOWARD         1              //馬達前進  
#define M_BACK           2              //馬達後退
#define M_ERROR          3              //馬達異常

//status_main 判斷狀態定義
#define F_INIT           0              //開機初始化 
#define F_IDLE           1              //待機中
#define F_ABORT          9              //退出

//status_batter 判斷狀態定義
#define B_IDLE             0              //NULL
#define B_WAIT_OBJECT_OUT  1              //判斷到物件後,馬達前進,等待離開2秒,time out則進入門口退出狀態 
#define B_WAIT_MENTAL_IN   2              //物件已進去,判斷是否為金屬物件
#define B_WAIT_LEN_IN      3              //等待電池進入長度檢查
#define B_WAIT_LEN_CHK     4              //檢查電池長度
#define B_WAIT_HEIGH_CHK   5              //檢查電池高度
#define B_RETUN_OBJECT_SET  6             //退出物件
#define B_RETUN_OBJECT     7              //退出物件
#define B_WAIT_DROP        8              //等待電池掉落
#define B_WAIT_CLR         9              //等待入口物件清除
#define B_PAUSE           10              //停止1秒
//#define B_CHK_SENSOR      13              //檢查感測器
#define B_RETUN_INIT      11              //初始強制退出2秒(約等於軌道長度)
#define B_RETUN_INIT_WAIT 12              //初始強制退出2秒
#define B_TIME_OUT        20              //錯誤-TIME OUT

//等待時間定義
#define TIME_OUT_ALL_MAX      500            //每個物件 馬達前進 最久為5秒
#define TIME_OUT_WAIT_M       300            //等待金屬時間 3秒
#define TIME_OUT_RETUN_OBJECT1 150           //退出物件最長時間 1.5秒
#define TIME_OUT_RETUN_OBJECT2 80            //退出物件時間,距離感測器前至入口 0.8秒
#define TIME_OUT_RETUN_OBJECT3 90            //退出物件時間,距離感測器後至入口 0.9秒
#define TIME_OUT_LEN_MAX      100            //等待電池長度感測進入1秒                //可以縮短  待測試
#define TIME_OUT_LEN_CHK      100            //等待電池長度感測離開檢查 1秒
#define TIME_OUT_HEIGH_CHK     40            //等待電池是否為9號時間 0.4秒 
#define TIME_OUT_WAIT_DROP     70            //等待電池掉落 0.7秒 
#define TIME_OUT_WAIT_DROP2    40            //等待電池掉落 0.4秒 
#define TIME_OUT_WAIT_ENTRY    20            //等待物件回到入口 0.08秒 
#define TIME_OUT_WAIT_PAUSE    60            //停止0.6秒,避免不斷作動
#define TIME_OUT_RETUN_INIT   200            //開始運作前強制退出2秒

//電池長度範圍
#define BT_LEN_MAX            400            //電池最大長度
#define BT_LEN_MIN            162            //電池最小長度
#define BT_LEN_34             290            //電池3號4號閥值
#define BT_LEN_46             258            //電池4號6號閥值
#define BT_LEN_65             200            //電池6號5號閥值
#define BT_LEN_12             330            //電池1號2號閥值

//金屬與長度 差異範圍
#define MENTAL_LEN_MAX        120            //最大差異
#define MENTAL_LEN_MIN        50             //最小差異

//堵轉標準值
#define STALL_MIN             150            //低於150 判斷為堵轉 
#define MENTAL_IN_MAX          60            //60*10ms 判斷為異常



struct KEY_Object
{
	unsigned char KeyCode;              /* 所有感測器及時狀態 */	
   unsigned int  Timer[SENSOR_MAX];    /* 所有感測器除彈跳後計數值 *10ms */ 		 
   
   union
   {
      unsigned char Value;   /* 所有感測器除彈跳後訊號值 */
            
      struct
      {
         unsigned int POS         : 1;   /* bit 1 */
         unsigned int LEN         : 1;   /* bit 2 */
         unsigned int HEIGH       : 1;   /* bit 3 */
         unsigned int MENTAL      : 1;   /* bit 4 */                   
      } B;            
      
      
   } DB_LOW;        
   
};
typedef struct KEY_Object KEY_Obj;

// 電池狀態結構
typedef struct {
    int16_t status;          // 電池位置狀態
    int16_t status_last;     // 電池位置之前狀態    
	 int16_t timer_pos;       // 感測到物件的時間    
	 int16_t timer_mental;    // 感測到物件是金屬的時間    
	 int16_t timer_len;       // 感測物件長度的時間    
	 int16_t timer_heigh;     // 感測物件高度的時間    	 
	 
} Batter_status_t;

// DEVICE狀態結構
typedef struct {
   int16_t status_motor;  	                     //馬達狀態
   int16_t flag_dir;                            //馬達前進後退旗標 0/前進;1/後退
   int16_t battery_count[BATTERY_TYPE_MAX];     //接收電池數量 
   int16_t sensor[SENSOR_MAX];                  //感測器狀態 0:位置感測 1:長度感測 2:高度感測 3:金屬感測
   int16_t stalled_cnt;  	                     //馬達堵轉計數
   int16_t error_flag;                          //馬達錯誤旗標 // bit0 = S2VSB, bit1 = S2VSA, bit2 = S2GB, bit3 = S2GA, bit4 = OTPW, bit5 = OT
	 
} Device_status_t;

extern Device_status_t devicd_struct;        

extern int16_t Device_Test(void);                                                                
extern int16_t Device_Run(void);
extern void Device_Close(void);
extern int16_t GetMotorErrorCode(void);                                                                

void Count_init(void);
void SensorValue_init(void);
int16_t track_clr(void);
void Process_fw(void);
void Process_fw_close(void);
void Process_LED(void);
void Process_tmc2240_current(void);
void Process_Sensor(void);
void Process_Main(void);
void gpioCallback(int gpio, int level, uint32_t tick);
void battery_clr(int number);
unsigned char read_sensor(void);
void IncorrectCheck(void);   
int16_t sensor_r_check(void);
int16_t sensor_s_check(void);
void laser_open(void);
void laser_close(void);

#endif // PROC_FW_H
