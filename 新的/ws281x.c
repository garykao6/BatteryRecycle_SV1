/**
 * @file WS281x.c
 * @brief 控制 WS2812B RGB
 * @version 1.0
 * @date 2025/8/21
 * @author Gary 
 *
 *
 * 系統設定：
 * - 使用 PWM1 CH0 控制 WS2812B（對應 GPIO12）
 * - 使用 DMA 通道 10 傳輸資料
 * - 使用 LED_COUNT 定義 WS2812 燈珠數量（預設為 33 顆）
 *
 * 程式特點：
 * - 利用 ws2811_render() 即時更新燈珠顯示狀態
 * - 結束時會清除所有燈號並釋放資源
 *
 * 注意事項：
 * - 確保 PWM 與 DMA 不與其他外設衝突
 */

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <gpiod.h>
#include <ws2811/ws2811.h>
#include <pthread.h>
#include "ws281x.h" 

#define LED_COUNT 33                                                                                                    
#define GPIO_PIN  18                                                                                                    

#define DMA 10
#define TRAIL_LENGTH 10  // 設定光條長度

int ledx33_mode, ledx33_mode_last;                          //0:全暗 1:彩虹燈  2:綠色呼吸燈 3:單條藍流水 4:雙條藍流水 5:紅色閃爍
static pthread_t ledx33_thread;
static int ledx33_thread_running = 0;
int offset = 0;
int pos1 = 0;                 // 順時針光條位置
int pos2 = LED_COUNT / 2;     // 逆時針光條位置 (對向)    
int step = 0;
uint32_t color_last = 0;
 
ws2811_t ledstring = {
    .freq = WS2811_TARGET_FREQ,
    .dmanum = DMA,
    .channel = {
        [0] = {
                .gpionum    = GPIO_PIN,
                .count      = LED_COUNT,
                .invert     = 0,
                .brightness = 25,
        },
        [1] = {.gpionum = 0}

    }
};

uint32_t color_fade(uint8_t r, uint8_t g, uint8_t b, float brightness)
{
    return ((uint32_t) (r * brightness) << 16) |
           ((uint32_t) (g * brightness) << 8) |
           ((uint32_t) (b * brightness));
}


// 彩虹顏色計算
static uint32_t Wheel(int pos) {
    pos = 255 - pos;
    if (pos < 85) {
        return ((255 - pos * 3) << 16) | (0 << 8) | (pos * 3);   // 紅→藍
    } else if (pos < 170) {
        pos -= 85;
        return (0 << 16) | (pos * 3 << 8) | (255 - pos * 3);     // 藍→綠
    } else {
        pos -= 170;
        return (pos * 3 << 16) | (255 - pos * 3 << 8) | 0;       // 綠→紅
    }
}

// 產生藍色 (帶亮度比例)
uint32_t blue_with_brightness(float scale)
{
    int b = (int)(255 * scale);
    if (b > 255) b = 255;
    if (b < 0)   b = 0;
    return (0x000000 | b);   // 0x0000FF 基礎藍
}

void Led_clr(void)                                   //0:全暗 
{
   for (int i = 0; i < LED_COUNT; i++)
      ledstring.channel[0].leds[i] = 0;
      
   ws2811_render(&ledstring);	
}

void rainbow_step(void)                              //1:彩虹燈
{
	
   for (int i = 0; i < LED_COUNT; i++) {
      int color_index = (i * 256 / LED_COUNT + offset) & 255;
      ledstring.channel[0].leds[i] = Wheel(color_index);
   }

  ws2811_render(&ledstring);
  usleep(100000);  // 100ms

  offset = (offset + 5) & 255; // 流動速度
          	
}

void breathing_step(uint32_t base_color)             //2:綠色呼吸燈
{
    static float b = 0;
    static int dir = 1;
    uint32_t c = color_fade((base_color >> 16) & 0xFF,(base_color >> 8) & 0xFF,base_color & 0xFF, b);
    
    for (int i = 0; i < LED_COUNT; i++)
        ledstring.channel[0].leds[i] = c;
    ws2811_render(&ledstring);
    usleep(30000);
    b += 0.02f * dir;
    if (b >= 1.0f) {
        b = 1.0f;
        dir = -1;
    }                                                                              // 控制最大亮度
    if (b <= 0.1f) {
        b = 0.1f;
        dir = 1;
    }                                                                               // 控制最小亮度
}

void flowing_one(void)                               //3:單條藍流水
{
        // 先清空
        for (int i = 0; i < LED_COUNT; i++) {
            ledstring.channel[0].leds[i] = 0x000000;
        }

        // 畫出光條 (頭亮、尾巴漸暗)
        for (int t = 0; t < TRAIL_LENGTH; t++) {
            int index = (offset - t + LED_COUNT) % LED_COUNT; // 環形索引
            float scale = 1.0 - (float)t / TRAIL_LENGTH;   // 漸暗比例
            ledstring.channel[0].leds[index] = blue_with_brightness(scale);
        }

        ws2811_render(&ledstring);
        usleep(30000); // 速度 (30ms)

        offset = (offset + 1) % LED_COUNT;
        
}

void flowing_two(void)                               //4:雙條藍流水
{
   // 清空所有 LED
   for (int i = 0; i < LED_COUNT; i++) {
       ledstring.channel[0].leds[i] = 0x000000;
   }

   // 畫出光條 1 (順時針)
   for (int t = 0; t < TRAIL_LENGTH; t++) {
       int index = (pos1 - t + LED_COUNT) % LED_COUNT;
       float scale = 1.0 - (float)t / TRAIL_LENGTH;
       ledstring.channel[0].leds[index] = blue_with_brightness(scale);
   }

   // 畫出光條 2 (逆時針)
   for (int t = 0; t < TRAIL_LENGTH; t++) {
       int index = (pos2 + t) % LED_COUNT;
       float scale = 1.0 - (float)t / TRAIL_LENGTH;
       ledstring.channel[0].leds[index] = blue_with_brightness(scale);
   }

   ws2811_render(&ledstring);
   usleep(30000); // 速度 (30ms)

   // 更新位置
   pos1 = (pos1 + 1) % LED_COUNT;                 // 順時針
   pos2 = (pos2 - 1 + LED_COUNT) % LED_COUNT;     // 逆時針	
}

void led_alarm(void)                                 //5:紅色閃爍
{
   for (int i = 0; i < LED_COUNT; i++)         //全暗
      ledstring.channel[0].leds[i] = 0;      
   ws2811_render(&ledstring);	
   usleep(400000);  // 400ms
   
   for (int i = 0; i < LED_COUNT; i++)         //紅色
      ledstring.channel[0].leds[i] = 0x00ff00;      
   ws2811_render(&ledstring);	
   usleep(200000);  // 200ms
   
   for (int i = 0; i < LED_COUNT; i++)         //全暗
      ledstring.channel[0].leds[i] = 0;      
   ws2811_render(&ledstring);	
   usleep(200000);  // 200ms
   
   for (int i = 0; i < LED_COUNT; i++)         //紅色
      ledstring.channel[0].leds[i] = 0x00ff00;      
   ws2811_render(&ledstring);	
   usleep(200000);  // 200ms                  	
}

void led_power(void)                                 //6:波動
{
	
     for (int i = 0; i < LED_COUNT; i++) {
         float wave = (sin((i + step) * M_PI / 8.0) + 1.0) / 2.0; // 0~1
         ledstring.channel[0].leds[i] = blue_with_brightness(wave);
     }

     ws2811_render(&ledstring);
     usleep(80000); // 控制波動速度

     step = (step + 1) % LED_COUNT;
}

void led_Red(void)                                   //7:紅色
{
   for (int i = 0; i < LED_COUNT; i++)         //全暗
      ledstring.channel[0].leds[i] = 0x00ff00;      
   ws2811_render(&ledstring);	              	
}

void led_Green(void)                                 //8:綠色
{
   for (int i = 0; i < LED_COUNT; i++)         //全暗
      ledstring.channel[0].leds[i] = 0xff0000;      
   ws2811_render(&ledstring);	              	
}

void led_Blue(void)                                 //9:藍色
{
   for (int i = 0; i < LED_COUNT; i++)         //全暗
      ledstring.channel[0].leds[i] = 0x0000ff;      
   ws2811_render(&ledstring);	              	
}

void led_color_set(uint32_t color)                        //color = R G B
{	
	if(color != color_last)	
	{
      for (int i = 0; i < LED_COUNT; i++)         
         ledstring.channel[0].leds[i] = color;      
      ws2811_render(&ledstring);			
	}	
   
   color_last = color;               	
}

static void* ledx33_blink_thread(void* arg)          //LED執行緒 
{
//    (void)arg;   // 告訴編譯器「不用這參數」    
    
    Led_init();
    for (int i = 0; i < LED_COUNT; i++)
        ledstring.channel[0].leds[i] = 0;        
    ws2811_render(&ledstring);
    
    ledx33_mode = 1;    
    
    while (ledx33_thread_running) {

        ledx33_mode_last = ledx33_mode;                

        switch (ledx33_mode) {
            case 0:
            	 Led_clr();                
                break;
            case 1:
                rainbow_step();
                break;
            case 2:
//                breathing_step(0x008080);
                breathing_step(0xff0000);
                break;
            case 3:
//                flowing_one();
                led_power();
                break;
            case 4:        	
					 flowing_two();
                break;
            case 5:        	
					 led_alarm();
                break;                
            case 6:        	
					 led_Red();
                break;   
            case 7:        	
					 led_Green();
                break; 
            case 8:        	
					 led_Blue();
                break;                                 
        }
        
        if(ledx33_mode_last != ledx33_mode)          //模式改變 需初始化變數
        {
        	  Led_clr();
           Led_init();	
        }	
        	
    }
    
    return NULL;
}

int16_t Led_Run(void)                                //LED程序打開
{

   if (ws2811_init(&ledstring) != WS2811_SUCCESS) {
   	 printf("ws2811_init failed\n\n");	
       return 1;
   }
   else
   	printf("ws2811_init ok\n\n");	   	
   
//   while(1)
//   {
//      led_Blue();  
//      usleep(80000); 	
//	}

//   ledx33_thread_running = 1;
//   pthread_create(&ledx33_thread, NULL, ledx33_blink_thread, NULL);
      
   return 0;  	
}

void Led_init(void)                                  //LED程序初始化
{
   offset = 0;
   pos1 = 0;                 // 順時針光條位置
   pos2 = LED_COUNT / 2;     // 逆時針光條位置 (對向)  	
   
}

void Led_close(void)                                 //LED程序關閉
{			
	printf("led clr \n\n");	
   Led_clr();
//   ledx33_thread_running = 0;
//	printf("ledx33_thread close \n\n");	   
//   pthread_join(ledx33_thread, NULL);   //關閉
   ws2811_fini(&ledstring);             //關閉ws2811 lib	 
}  


