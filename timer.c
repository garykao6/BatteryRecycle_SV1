#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "timer.h"


int16_t TimeArray[TIMER_MAX];


void InitTimer(void)
{
   int8_t i;  
   
   for(i = 0; i < TIMER_MAX; i++)
      TimeArray[i] = 0;		
	
//	printf("Init Timer \r\n");
}

void Process_Timer(void)
{
   int16_t i;  
   
   for(i = 0; i < TIMER_MAX; i++)
   {
      if(TimeArray[i] > 0) 
         TimeArray[i]--;	         
   }	
}

