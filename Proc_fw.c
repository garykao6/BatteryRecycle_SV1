//2025/9/25 下午 02:20:43 V0.15
//1.if(LEN_STATUS == 1) 除彈跳 1ms 改成10ms
//2.退出紅燈號改成藍色
//3.B_WAIT_CLR 退出機制加上retry
//2025/9/30 下午 01:50:41
//調整LED燈號時機
//2025/10/2 下午 02:03:45
//1.更換長度與入口感測器腳位
//2025/10/28 下午 05:46:26
//v0118 增加條件 可剔除16340
//2025/10/29 下午 04:44:29
//v0119 1.增加中斷除彈跳(因變形電池而退出的機率減少) 2.退出重試次數5次改成10次
//v0.31 開機自檢 測試OK
//v0.32 SensorValue.DB_LOW.B.MENTAL = 0;                 //v0.32 新增   避免B_RETUN_OBJECT_3  ,測試未確認
//


#include "Proc_fw.h"
#include "Motor.h"
#include "Gpio.h"
#include <pigpio.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include "timer.h"
#include <stdbool.h>
#include "log.h"
#include <stdlib.h>

//設定除錯訊息是否輸出-正常狀態
//#define _DEBUG_OBJECT_NORMAL
#define _DEBUG_OBJECT_ERROR

//B_RETUN_OBJECT_1 退出原因1: 金屬未感測 結果長度先感測到,判斷非電池     //OK
//B_RETUN_OBJECT_2 退出原因2: 金屬感測TIME OUT,量測不到訊號 異常處理    //未使用
//B_RETUN_OBJECT_3 退出原因3: 長度感測TIME OUT,量測不到訊號 異常處理   //OK
//B_RETUN_OBJECT_4 退出原因4: 金屬感測大於600ms 異常處理              //OK
//B_RETUN_OBJECT_5 退出原因5: 低電池長度不正確                       //OK
//B_RETUN_OBJECT_6 退出原因6: 電池長度TIME OUT,量測不到電池長度,大概是電池卡在長度感測 //OK
//B_RETUN_OBJECT_7 退出原因7: 電池長度與金屬長度差異異常,可能是異常拉桿
//B_RETUN_OBJECT_8 退出原因5: 高電池長度不正確

static pthread_t object_thread;
static int object_thread_running = 0;
int flag_led = 0;

#define NUM_PINS 2
int pins[NUM_PINS] = {PIN_LEN_R, PIN_MENTAL_SENSOR};  //BCM 腳位編號
uint32_t low_start[NUM_PINS] = {0};
uint32_t low_duration[NUM_PINS] = {0};

int status_motor_last, debug_status_last, sg_result_last;
Device_status_t devicd_struct;
Batter_status_t Bt_struct[2];
KEY_Obj SensorValue;
uint8_t  cnt_retry;

void Device_Open(void)                                                                
{
   printf( "韌體版本 V0.32 \n");
   Hardware_Init();                                                                    //初始化GPIO
   
}

int16_t Device_Run(void)                                                                
{
   int16_t code_error;

   atexit(log_close);  // 程式結束自動關檔
   log_write("Device Run");

   Led_Select(0);	                                                                   //測試中不亮
   code_error = Tmc2240_init();                                                        //初始化TMC2240
   if(code_error > 0)
      return code_error;

   code_error = sensor_r_check();                                                      //檢查雷射接收
   if(code_error > 0)
     return code_error;
          
   if(code_error == 0)
      Process_fw();
      
   return code_error;  	
}

void Device_Stop(void)
{
   Count_init();
   Process_fw_close();	

   gpioWrite(EN_PIN, 1); // 預設關閉驅動器
   gpioWrite(CS_PIN, 1);   

   gpioWrite(PIN_HEIGH_LP, 0);         
   gpioWrite(PIN_POSITION_LP, 0);    
   gpioWrite(PIN_LEN_LP, 0);  	

   log_write("Device Stop");
   log_close();
}

void Device_Close(void)
{

   Hardware_Close();
	
}

// 定時器回呼函數（每隔一段時間被呼叫）
void process_toggle(void)                     //10ms
{

    Process_Timer();   

}

//工作燈
void Process_LED(void)                        //100ms
{   
   if(TimeArray[timer_led_sw] == 0)       
   {
   	TimeArray[timer_led_sw] = 10;            //10ms * 10 = 100ms

      flag_led = !flag_led;
      gpioWrite(PIN_SW_LED, flag_led);   		
   }	
		
}

//SENSOR
void Process_Sensor(void)                     //10ms
{   
   unsigned char i,j;	
   if(TimeArray[timer_sensor_task] == 0)       
   {
   	TimeArray[timer_sensor_task] = 1;          //10ms

      SensorValue.KeyCode = read_sensor();    
      j = 1;
      for(i = 0;i<SENSOR_MAX;i++)  
      {    	   	
         if(SensorValue.KeyCode & j) 
         {         
    
            if (SensorValue.Timer[i] < 200)
               SensorValue.Timer[i]++;
            
            if (SensorValue.Timer[i] == PULSE_WIDTH) 	    /* 10ms */ 
            {         
               SensorValue.DB_LOW.Value |= j;               
                   
            }                                           
         }	              
         else
         {
            SensorValue.Timer[i] = 0;     	    	 
         }	 
        	       	 
         j <<= 1;      	 
   	}	
   	      
   }	 
                                                       
}

void Process_Main(void)                                       //LOOP
{            
	uint32_t low_duration_temp = 0;
	
	Process_Sensor();
	
    if(Bt_struct[0].status  == B_IDLE)                           //無感測到任何物件,改使用event
    {

		if(SensorValue.DB_LOW.B.MENTAL)                         //確認到金屬感測訊號
		{                                                       //此時間如還有電池在軌道的處理
            SensorValue.DB_LOW.B.MENTAL = 0;

			Bt_struct[0].status = B_WAIT_LEN_IN;
			TimeArray[timer_timeout1] = TIME_OUT_WAIT_M;         //感測到金屬後 等待長度感測1.5秒
            Led_Select(3);                                      //藍色-開始判斷狀態
			printf( "B_WAIT_LEN_IN  \n");

			if(Bt_struct[0].timer_mental <= 200)                 //計數值最高200
               Bt_struct[0].timer_mental++;

            IncorrectCheck();                                       //異常動作判斷
		}
		else if(SensorValue.DB_LOW.B.LEN)                       //長度感測器感測到電池
		{                                                       //未感測到金屬的處理
            SensorValue.DB_LOW.B.LEN = 0;
			Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
            devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
            TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT1;    //退出設定2.退出時間設定

            #ifdef _DEBUG_OBJECT_ERROR
			printf( "B_RETUN_OBJECT_1A \n");
			#endif
			if(Bt_struct[0].timer_len <= 200)                 //計數值最高200
               Bt_struct[0].timer_len++;
		}
		else if(SensorValue.DB_LOW.B.POS)                       //感測到物件
		{
            SensorValue.DB_LOW.B.POS = 0;

			Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
            devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
            TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT1;    //退出設定2.退出時間設定

            #ifdef _DEBUG_OBJECT_ERROR
			printf( "B_RETUN_OBJECT_1B \n");
			#endif
			if(Bt_struct[0].timer_pos <= 200)                 //計數值最高200
               Bt_struct[0].timer_pos++;
		}
		else if(SensorValue.DB_LOW.B.HEIGH)                       //感測到物件
		{
            SensorValue.DB_LOW.B.HEIGH = 0;

			Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
            devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
            TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT1;    //退出設定2.退出時間設定

            #ifdef _DEBUG_OBJECT_ERROR
			printf( "B_RETUN_OBJECT_1C \n");
			#endif
		}
		else
		{
// 			Bt_struct[0].status = B_WAIT_MENTAL_IN;
			devicd_struct.status_motor = M_FOWARD;
// 			TimeArray[timer_timeout1] = TIME_OUT_WAIT_M;         //每個物件 馬達前進 最久為5秒
// 			printf( "B_IDLE \n");
		}
   	
//    	IncorrectCheck();                                       //異常動作判斷
    }

	else if(Bt_struct[0].status  == B_WAIT_LEN_IN)			   //等待長度感測訊號開始
	{

		if(SensorValue.DB_LOW.B.LEN)
		{
			SensorValue.DB_LOW.B.LEN = 0;

			Bt_struct[0].status = B_WAIT_LEN_CHK;
			TimeArray[timer_timeout1] = TIME_OUT_LEN_CHK;           //等待電池長度感測 馬達前進 最久為2秒
			#ifdef _DEBUG_OBJECT_NORMAL
			printf( "B_WAIT_LEN_CHK \n");
			#endif
		}
		else if(TimeArray[timer_timeout1] == 0)
		{
			Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
			devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
			TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT1;    //退出設定2.退出時間設定

			#ifdef _DEBUG_OBJECT_ERROR
			printf( "B_RETUN_OBJECT_3 \n");
			#endif
		}
   	
	}	

	else if(Bt_struct[0].status  == B_WAIT_LEN_CHK)	           //等待長度感測訊號結束
	{
         if(SensorValue.Timer[3] >= MENTAL_IN_MAX)               //金屬大於600ms 異常處理
         {
            SensorValue.DB_LOW.B.POS = 0;                         //退出前忽略其他進入物品

            Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
            devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
            TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT4;    //退出設定2.退出時間設定

            #ifdef _DEBUG_OBJECT_ERROR
            printf( "B_RETUN_OBJECT_4 \n");
            #endif
         }
         else if(LEN_STATUS == 1)
         {
      //   		usleep(1000);                         //簡易除彈跳
            usleep(15000);                         //簡易除彈跳
            if(LEN_STATUS == 1)                   //確認到長度感測訊號
            {
      //   			#ifdef _DEBUG_OBJECT_NORMAL
      //            printf("電池長度: %d ms ,金屬長度: %d ms\n", low_duration[0], low_duration[1]);
      //            #endif

                  if(low_duration[1] >= low_duration[0])
                     low_duration_temp = low_duration[1] - low_duration[0];
                  printf("電池長度: %d ms ,金屬長度: %d ms , 差異長度: %d ms\n " , low_duration[0], low_duration[1] ,low_duration_temp);


//                   if((low_duration_temp<=MENTAL_LEN_MIN)||(low_duration_temp>=MENTAL_LEN_MAX))        //金屬與長度 差異應在合理範圍, (>40 & <130)
//                   {
//                      SensorValue.DB_LOW.B.POS = 0;                           //退出前忽略其他進入物品
//
//                      Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
//                      devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
//                      TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT3;    //退出設定2.退出時間設定
//
//                      #ifdef _DEBUG_OBJECT_ERROR
//                      printf( "B_RETUN_OBJECT_7 \n");
//                      #endif
//                   }

                  if((low_duration[0] < BT_LEN_MIN)||(low_duration[0] > BT_LEN_MAX))
                  {
                     SensorValue.DB_LOW.B.POS = 0;                           //退出前忽略其他進入物品

                     Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
                     devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
                     TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT3;    //退出設定2.退出時間設定

                     #ifdef _DEBUG_OBJECT_ERROR
                     printf( "B_RETUN_OBJECT_5 電池長度: %d ms \n", low_duration[0]);
                     #endif
                  }
                  else                                                     //電池長度正確 待確認是否為9號電池
                  {
                     Bt_struct[0].status = B_WAIT_HEIGH_CHK;
                     TimeArray[timer_timeout1] = TIME_OUT_HEIGH_CHK;       //檢查完畢把電池帶入時間  0.3秒

        			 #ifdef _DEBUG_OBJECT_NORMAL
                     printf( "B_WAIT_HEIGH_CHK \n");
                     #endif
                  }
            }

         }
         else if	(TimeArray[timer_timeout1] == 0)
         {
            Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
            devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
            TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT6;    //退出設定2.退出時間設定

            #ifdef _DEBUG_OBJECT_ERROR
            printf( "B_RETUN_OBJECT_6 \n");
            #endif
         }
		
	}	

	else if(Bt_struct[0].status  == B_WAIT_HEIGH_CHK)	        //電池高度判斷
	{				
         if	(TimeArray[timer_timeout1] == 0)
         {
            Bt_struct[0].status =  B_WAIT_DROP;
            TimeArray[timer_timeout1] = TIME_OUT_WAIT_DROP2;             //等待電池掉落 0.2秒


            if(low_duration[0] >= BT_LEN_34)                             //使用電池長度判斷是否是3號電池
            {
               devicd_struct.battery_count[BT_NUM_3]++;
               #ifdef _DEBUG_OBJECT_NORMAL
               printf("電池3號: %d \n", devicd_struct.battery_count[BT_NUM_3]);
               #endif
               log_write("BT_NUM_3 : %d ", devicd_struct.battery_count[BT_NUM_3]);
            }
            else if(low_duration[0] >= BT_LEN_46)                        //使用電池長度判斷是否是4號電池
            {
               devicd_struct.battery_count[BT_NUM_4]++;
               #ifdef _DEBUG_OBJECT_NORMAL
               printf("電池4號: %d \n", devicd_struct.battery_count[BT_NUM_4]);
               #endif
               log_write("BT_NUM_4 : %d ", devicd_struct.battery_count[BT_NUM_4]);
            }
            else if(low_duration[0] >= BT_LEN_65)                        //使用電池長度判斷是否是6號電池
            {
               devicd_struct.battery_count[BT_NUM_6]++;
               #ifdef _DEBUG_OBJECT_NORMAL
               printf("電池6號: %d \n", devicd_struct.battery_count[BT_NUM_6]);
               #endif
               log_write("BT_NUM_6 : %d ", devicd_struct.battery_count[BT_NUM_6]);
            }
            else
            {
               devicd_struct.battery_count[BT_NUM_5]++;
               #ifdef _DEBUG_OBJECT_NORMAL
               printf("電池5號: %d \n", devicd_struct.battery_count[BT_NUM_5]);
               #endif
               log_write("BT_NUM_5 : %d ", devicd_struct.battery_count[BT_NUM_5]);
            }


         }
         else if(SensorValue.DB_LOW.B.HEIGH)                             //高度感測器感測到電池
         {
            SensorValue.DB_LOW.B.HEIGH = 0;

            if(low_duration[0] >= BT_LEN_N1)                             //1號電池
            {
               devicd_struct.battery_count[BT_NUM_1]++;

               Bt_struct[0].status =  B_WAIT_DROP;
               TimeArray[timer_timeout1] = TIME_OUT_WAIT_DROP;              //等待電池掉落 0.4秒
               #ifdef _DEBUG_OBJECT_NORMAL
               printf("電池1號: %d \n", devicd_struct.battery_count[BT_NUM_1]);
               #endif
               log_write("BT_NUM_1 : %d ", devicd_struct.battery_count[BT_NUM_1]);
            }
            else if(low_duration[0] >= BT_LEN_N2)                        //2號電池
            {
               devicd_struct.battery_count[BT_NUM_2]++;

               Bt_struct[0].status =  B_WAIT_DROP;
               TimeArray[timer_timeout1] = TIME_OUT_WAIT_DROP;              //等待電池掉落 0.4秒
               #ifdef _DEBUG_OBJECT_NORMAL
               printf("電池2&9號: %d \n", devicd_struct.battery_count[BT_NUM_2]);
               #endif
               log_write("BT_NUM_2 : %d ", devicd_struct.battery_count[BT_NUM_2]);
            }
            else                                                            //其他物件,或16340
            {
               SensorValue.DB_LOW.B.POS = 0;                           //退出前忽略其他進入物品

               Bt_struct[0].status = B_RETUN_OBJECT_SET;               //退出
               devicd_struct.battery_count[BT_OBJ]++;                  //退出設定1.退出種類上數
               TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT8;    //退出設定2.退出時間設定

               #ifdef _DEBUG_OBJECT_ERROR
               printf( "B_RETUN_OBJECT_8 \n");
               #endif

            }

         }
				
    }

	else if(Bt_struct[0].status  == B_WAIT_DROP)	            //等待電池掉落
	{
      if	(TimeArray[timer_timeout1] == 0)
      {
//           devicd_struct.status_motor = M_STOP;
          Bt_struct[0].status =  B_IDLE;
          battery_clr(0);                                                 //待確認
          //    SensorValue.DB_LOW.B.LEN = 0;
          SensorValue.DB_LOW.B.POS = 0;
          SensorValue.DB_LOW.B.HEIGH = 0;
          SensorValue.DB_LOW.B.MENTAL = 0;                 //v0.32 新增   避免B_RETUN_OBJECT_3  ,測試未確認

          Led_Select(2);                                   //綠色恆亮-等待投入狀態

          #ifdef _DEBUG_OBJECT_NORMAL
          printf( "FINISH \n");
          #endif
      }
      else if(SensorValue.DB_LOW.B.MENTAL)                    //確認到金屬感測訊號, 可能下一個投入太快
      {                                                       //此時間如還有電池在軌道的處理
//          SensorValue.DB_LOW.B.MENTAL = 0;                        //應該清除一個進入訊號
//          devicd_struct.status_motor = M_FOWARD;
//                Bt_struct[0].status = B_WAIT_MENTAL_IN;

//          #ifdef _DEBUG_OBJECT_NORMAL
//          printf( "B_WAIT_DROP_ELSE2 \n");
//          #endif
      }
      else if(SensorValue.DB_LOW.B.POS)                       //感測到物件
      {
//             Bt_struct[0].status = B_IDLE;                        //需修改,測試中
//             //Led_Select(2);                                   //綠色恆亮-等待投入狀態
//             #ifdef _DEBUG_OBJECT_NORMAL
//             printf( "B_WAIT_DROP_ELSE1 \n");
//             #endif
      }
		
	}	

	else if(Bt_struct[0].status  == B_RETUN_OBJECT_SET)		    //退出物件前設定
	{		
       Led_Select(1);                                          //紅色恆亮-不能投入狀態

   	   devicd_struct.status_motor = M_BACK;
       Bt_struct[0].status = B_RETUN_OBJECT;
	   cnt_retry = 0; 	   	
	}	
	
	else if(Bt_struct[0].status  == B_RETUN_OBJECT)			    //退出物件
	{		
      if(TimeArray[timer_timeout1] == 0)                         //強制跑完固定時間
      {
          Bt_struct[0].status = B_WAIT_CLR;
          TimeArray[timer_timeout1] = TIME_OUT_WAIT_ENTRY;          //等待物件退出入口 0.1秒

          #ifdef _DEBUG_OBJECT_NORMAL
          printf( "B_RETUN_OBJECT  %d\n", devicd_struct.flag_dir );
          #endif

      }
	   	
	}

	else if(Bt_struct[0].status  == B_WAIT_CLR)			  
	{
   	if(cnt_retry >= RETRY_MAX)  		
   	{
   	   devicd_struct.status_motor = M_STOP;
   	   Bt_struct[0].status = B_IDLE;   
   	   Led_Select(2);                                   //綠色恆亮-等待投入狀態
   	}			
   	else if(LEN_STATUS == 0)                    //長度持續感測到物件需要持續退出,但可能照成退不停狀態 
   	{
   		 usleep(1000);                        //簡易除彈跳
   		 if(LEN_STATUS == 0) 
   		 {
             Bt_struct[0].status = B_RETUN_OBJECT;                        //退出   	      
   	       TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT3;         //退出時間設定
   	       
   	       cnt_retry = cnt_retry + 1;  
   		    #ifdef _DEBUG_OBJECT_ERROR
   		 	 printf( "DEBUG 1, cnt: %d\n ", cnt_retry);  	         	   	   
   		    #endif   	         		 	
		 	
   		 }
   	}
   	else if(MENTAL_STATUS == 0)                 //金屬持續感測到物件需要持續退出,但可能照成退不停狀態 
   	{
   		 usleep(1000);                        //簡易除彈跳
   		 if(MENTAL_STATUS == 0) 
   		 {
             Bt_struct[0].status = B_RETUN_OBJECT;                        //退出   	      
   	       TimeArray[timer_timeout1]  = TIME_OUT_RETUN_OBJECT2;         //退出時間設定    
   	       
   	       cnt_retry = cnt_retry + 1;		 	
   		    #ifdef _DEBUG_OBJECT_ERROR
   		 	 printf( "cnt_retry, cnt: %d\n ", cnt_retry);
   		    #endif  	
   		 }
   	}
   	else if(POS_STATUS == 1)                    //沒感測到物件   
   	{
   		usleep(1000);                         //簡易除彈跳 
   		if(POS_STATUS == 1)                   //沒確認到物件 
   		{
   	      Bt_struct[0].status = B_PAUSE;  
   	      devicd_struct.status_motor = M_STOP;    
   	      TimeArray[timer_timeout1] = TIME_OUT_WAIT_PAUSE;
   	      #ifdef _DEBUG_OBJECT_NORMAL
   		   printf( "B_WAIT_CLR \n");  
   		   #endif 		   	
   		}   		
   	}   		   		
   	else if(TimeArray[timer_timeout1] == 0)  		
   	{
   	   devicd_struct.status_motor = M_STOP;
//   	   Bt_struct[0].status = B_IDLE;    
   	}	
		
	}		

	else if(Bt_struct[0].status  == B_PAUSE)
	{
   	if(TimeArray[timer_timeout1] == 0)   		
   	{
   	   Bt_struct[0].status = B_IDLE;
   	   Led_Select(2);                                   //綠色恆亮-等待投入狀態
   	   SensorValue_init();  
   	   #ifdef _DEBUG_OBJECT_NORMAL
   		printf( "B_IDLE \n");  
   		#endif
   	}			
		
	}			   		
	
	
	//
	else if(Bt_struct[0].status  == B_RETUN_INIT)		        //退出物件前設定
	{
        //Led_Select(1);                                //紅色恆亮-不能投入狀態

    	devicd_struct.status_motor = M_BACK;
    	TimeArray[timer_timeout1] = TIME_OUT_RETUN_INIT;        //強制退出2秒
    	Bt_struct[0].status = B_RETUN_INIT_WAIT;
   	
	}	
	else if(Bt_struct[0].status  == B_RETUN_INIT_WAIT)		     //等待2秒
	{
		if(TimeArray[timer_timeout1] == 0)                      //強制跑完固定時間後 再用入口感測判斷停止
		{
//         devicd_struct.error_flag = sensor_s_check();                     //雷射接收檢查,評估是否檢查??
//         if(devicd_struct.error_flag > 0)
            if(sensor_s_check() > 0)
            {
               Bt_struct[0].status = B_RETUN_INIT;
               cnt_retry = cnt_retry + 1;
               printf("設備異常 %d\n",devicd_struct.error_flag);
               if(cnt_retry >= 5)
               {
                  devicd_struct.error_flag = sensor_s_check();

                  devicd_struct.status_motor = M_STOP;
                  Bt_struct[0].status = B_ERROR;
            //	   Led_Select(1);                                   //紅色恆亮
               }
            }
            else
            {
               battery_clr(0);
               Bt_struct[0].status = B_IDLE;
               Led_Select(2);                                   //綠色恆亮-等待投入狀態

               devicd_struct.status_motor = M_STOP;
               laser_open();                                        //打開雷射發射
               usleep(50000);                                        //DELAY
               Count_init();                                        //初始化計數暫存

               devicd_struct.error_flag = 0;						 //清0 表示檢測正常,可開始收電池
            }
        	         	
       
		}					
	}	
	else if(Bt_struct[0].status  == B_ERROR)		              //退出物件前設定	
	{
		
		
	}			
    else
    {

    }
  
// 	if(Bt_struct[0].status != debug_status_last)
// 	{
// 		//printf("status: %d \n", Bt_struct[0].status);
// 	}
//    debug_status_last = Bt_struct[0].status;

   if(status_motor_last != devicd_struct.status_motor)
   {
      if(devicd_struct.status_motor == M_FOWARD)
      {
         devicd_struct.flag_dir = true;       
         Motor_Foward();    	
         Motor_Run();                //馬達啟動
      }	
      else if(devicd_struct.status_motor == M_BACK) 
      {
         devicd_struct.flag_dir = false;              
         Motor_Back(); 
         Motor_Run();                //馬達啟動      
      }
      else//M_STOP
      {
         Motor_Stop();               //馬達停止
      }     	
   }	 
   status_motor_last = devicd_struct.status_motor;
	
}

static void* object_process_thread(void* arg)
{
    printf( "object thread starting\n");
	 devicd_struct.status_motor = M_STOP;
	 	 
    InitTimer();
    gpioSetTimerFunc(0, 10, process_toggle);	// 設定 Timer 編號=0，每 10ms 呼叫一次 process_toggle
    
    // 設定中斷監聽，任何電平變化都會觸發 gpioCallback
    for (int i = 0; i < NUM_PINS; i++)
    {
//        gpioSetMode(pins[i], PI_INPUT);
//        gpioSetPullUpDown(pins[i], PI_PUD_UP); // 若需要上拉
        gpioGlitchFilter(pins[i], 9000);                // 忽略 <9ms 的彈跳
        gpioSetAlertFunc(pins[i], gpioCallback);
    }

    Bt_struct[0].status = B_RETUN_INIT;
    devicd_struct.error_flag = 20;	      	                 // 檢測中
    cnt_retry = 0;   
    
    while (object_thread_running) 
    {    	      	  
        Process_Main();
        if(TimeArray[timer_read_current] == 0)       
        {
   	     TimeArray[timer_read_current] = 10;          //100ms
           Process_tmc2240_current();
        }   
        Process_LED();        
    }
    return NULL;
}
            
void Count_init(void)
{
   for (int i = 0; i < BATTERY_TYPE_MAX; i++)	
      devicd_struct.battery_count[i] = 0;	
   devicd_struct.stalled_cnt = 0;	   
//   devicd_struct.error_flag = 0;	      
   SensorValue_init();      
}      

void SensorValue_init(void)
{
   SensorValue.DB_LOW.Value = 0;                        //全部清除為0   
}

void Process_fw(void)
{		
    object_thread_running = 1;
    pthread_create(&object_thread, NULL, object_process_thread, NULL);
	
}    

void Process_fw_close(void)
{		
   object_thread_running = 0;
   pthread_join(object_thread, NULL);   //關閉
	
}  

// 中斷回呼函數
void gpioCallback(int gpio, int level, uint32_t tick)
{
    
    for (int i = 0; i < NUM_PINS; i++)
    {
        if (gpio == pins[i])
        {
            if (level == 0)  // 低電位開始
            {
                low_start[i] = tick;
            }
            else if (level == 1) // 回到高電位
            {
                if (low_start[i] != 0)
                {
                    uint32_t diff_us = tick - low_start[i];
                    low_duration[i] = diff_us / 1000; // 轉成毫秒
//                    if(i == 0)
//                     printf("GPIO %d 低電位持續時間: %u ms\n", gpio, low_duration[i]);
                    low_start[i] = 0;
                }
            }
        }
    }
}


void battery_clr(int number)
{	
   Bt_struct[number].status = B_IDLE; 
   Bt_struct[number].status_last = B_IDLE;    

   Bt_struct[number].timer_pos = 0; 
   Bt_struct[number].timer_mental = 0; 
   Bt_struct[number].timer_len = 0; 
   Bt_struct[number].timer_heigh = 0; 


}


unsigned char read_sensor(void)
{	 
   unsigned char port_in;
   
   port_in = 0;
   
   if (POS_STATUS == 0)
      port_in |= 0x01; 
   if (LEN_STATUS == 0)
      port_in |= 0x02; 
   if (HEIGH_STATUS == 0)
      port_in |= 0x04; 
   if (MENTAL_STATUS == 0)
      port_in |= 0x08; 

   devicd_struct.sensor[0] = POS_STATUS;        //更新回傳狀態
   devicd_struct.sensor[1] = LEN_STATUS;
   devicd_struct.sensor[2] = HEIGH_STATUS;
   devicd_struct.sensor[3] = MENTAL_STATUS;
          
	return port_in;
}

void Process_tmc2240_current(void)
{
      uint32_t val = TMC2240_ReadCurrent();
//      printf("Reg 0x6F = 0x%08X\n",val);	
      
      // 解析旗標 (依 TMC2240 / TMC2209 資料表)      
      int cs_actual  = (val >> 16) & 0xFF;   // [23:16]
      int sg_result  = val & 0x3FF;          // [9:0]
      
      if((sg_result < STALL_MIN)&&(sg_result_last >= STALL_MIN)) {                       //只抓一次變化
         //printf("TMC2240 狀態: CS_ACTUAL=%d, SG_RESULT=%d, RAW=0x%08X\n",cs_actual, sg_result, val);	         
         if(devicd_struct.stalled_cnt < 10000)                               //馬達堵轉計數
            devicd_struct.stalled_cnt++;
         else
         	devicd_struct.stalled_cnt = 0;         	
         printf("stalled count = %d\n",devicd_struct.stalled_cnt);		
      }	

//      devicd_struct.error_flag = (val >> 25) & 0x7F;  // 把 bit31..25 抓下來
      val = (val >> 25) & 0x7F;  // 把 bit31..25 抓下來

      if (val & (1 << 0)) devicd_struct.error_flag = 11; // OT    ,過溫保護觸發 
      if (val & (1 << 1)) devicd_struct.error_flag = 12; // OTPW  ,過溫預警S2VSA 
      if (val & (1 << 2)) devicd_struct.error_flag = 13; // S2GA  ,A相短路至GND
      if (val & (1 << 3)) devicd_struct.error_flag = 14; // S2GB  ,B相短路至GND
      if (val & (1 << 4)) devicd_struct.error_flag = 15; // OLA   ,A相短路至VM
      if (val & (1 << 5)) devicd_struct.error_flag = 16; // OLB   ,B相短路至VM
      
//      if(devicd_struct.error_flag > 0)      
      if(val > 0)      
      {
         devicd_struct.status_motor = M_ERROR;      	          //馬達停止
         printf("馬達異常 %d\n",devicd_struct.error_flag);		                  
      }	      

      sg_result_last = sg_result; 
//      float i_rms = (cs_actual + 1) / 32.0 * 2.0;       // 假設 IRUN=31 (2A), GLOBALSCALER=0
//      printf("估算馬達電流 = %.2f A RMS\n", i_rms);
          
		
}

void IncorrectCheck(void)                                       //釣魚動作判斷  
{
   if((Bt_struct[0].timer_pos > 0)&&(Bt_struct[0].timer_len > 0))	 //在IDLE狀態下,金屬與長度都有感測到訊號,判斷為釣魚動作
   {
      Bt_struct[0].timer_pos = 0;
      Bt_struct[0].timer_len    = 0;
      devicd_struct.battery_count[BT_ERR]++;                               //釣魚動作count
      printf("釣魚動作: %d \n", devicd_struct.battery_count[BT_ERR]);
   }	
	
}

int16_t sensor_r_check(void)                                    //雷射接收檢查     
{	
   int16_t code_error = 0;
   printf("sensor_r_check in \n" );

   gpioWrite(PIN_POSITION_LP, 0);       //雷射關閉
   gpioWrite(PIN_LEN_LP, 0);        
   gpioWrite(PIN_HEIGH_LP, 0);  

   if(POS_STATUS == 1)                     //雷射關閉不應該有訊號=1   	
   {
      usleep(1000);                        //簡易除彈跳 
   	if(POS_STATUS == 1)                  //雷射關閉不應該有訊號=1
   	{
         code_error = 4;
         printf("sensor_check code %d \n", code_error);
         return code_error; 	
      }
   }

   if(LEN_STATUS == 1)                     //雷射關閉不應該有訊號=1   	
   {
      usleep(1000);                        //簡易除彈跳 
   	if(LEN_STATUS == 1)                  //雷射關閉不應該有訊號=1
   	{
         code_error = 5;
         printf("sensor_check code %d \n", code_error);
         return code_error; 	
      }
   }

   if(HEIGH_STATUS == 1)                   //雷射關閉不應該有訊號=1   	
   {
      usleep(1000);                        //簡易除彈跳 
   	if(HEIGH_STATUS == 1)                //雷射關閉不應該有訊號=1 
   	{
         code_error = 6;
         printf("sensor_check code %d \n", code_error);
         return code_error; 	
      }
   }
   
   return 0; 	
}

int16_t sensor_s_check(void)                                    //雷射檢查     
{	
   int16_t code_error = 0;

   laser_open();
   
   if(POS_STATUS == 0)                     //有物品阻擋或發射器故障  	
   {
      usleep(1000);                        //簡易除彈跳 
   	if(POS_STATUS == 0)                  
   	{
         code_error = 7;
         printf("sensor_check code %d \n", code_error);
         laser_close();
         return code_error; 	
      }
   }

   if(LEN_STATUS == 0)                     //有物品阻擋或發射器故障   	
   {
      usleep(1000);                        //簡易除彈跳 
   	if(LEN_STATUS == 0)                  
   	{
         code_error = 8;
         printf("sensor_check code %d \n", code_error);
         laser_close();         
         return code_error; 	
      }
   }

   if(HEIGH_STATUS == 0)                   //有物品阻擋或發射器故障  	
   {
      usleep(1000);                        //簡易除彈跳 
   	if(HEIGH_STATUS == 0)                
   	{
         code_error = 9;
         printf("sensor_check code %d \n", code_error);
         laser_close();         
         return code_error; 	
      }
   }

   if(MENTAL_STATUS == 0)                   //有物品阻擋或感測器故障  	
   {
      usleep(1000);                        //簡易除彈跳 
   	if(MENTAL_STATUS == 0)                
   	{
         code_error = 10;
         printf("sensor_check code %d \n", code_error);

         return code_error; 	
      }
   }

   laser_close();
      
   return 0; 	
}

int16_t GetMotorErrorCode(void)
{    	
    return devicd_struct.error_flag; // 無異常	      
    
}

void laser_open(void)
{
   gpioWrite(PIN_POSITION_LP, 1);       //雷射打開
   gpioWrite(PIN_LEN_LP, 1);        
   gpioWrite(PIN_HEIGH_LP, 1);  	
	usleep(10000); 
}

void laser_close(void)
{
   gpioWrite(PIN_POSITION_LP, 0);       //雷射關閉
   gpioWrite(PIN_LEN_LP, 0);        
   gpioWrite(PIN_HEIGH_LP, 0);  	
	usleep(10000); 
}



