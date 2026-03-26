#ifndef _TIMER_H
#define _TIMER_H


	
#define TIMER_MAX            4
#define timer_led_sw         0          //工作燈         200ms
#define timer_timeout1       1          //馬達前進總時間最久5秒
#define timer_sensor_task    2          //sensor_task   60ms
#define timer_read_current   3          //讀取馬達電流    500ms


extern int16_t TimeArray[TIMER_MAX];

extern void InitTimer(void);
extern void Process_Timer(void);


#endif
