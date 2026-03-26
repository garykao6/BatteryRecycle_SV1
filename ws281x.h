#ifndef WS281X_H
#define WS281X_H

extern int ledx33_mode;

uint32_t color_fade(uint8_t r, uint8_t g, uint8_t b, float brightness);
void rainbow_step(void);
void rainbow_grouped_step(void);
void breathing_step(uint32_t base_color);
void Led_clr(void);
void led_alarm(void);
void Led_init(void);                                  //LEDµ{§Çªì©l¤Æ

extern int16_t Led_Run(void);
extern void Led_close(void);
extern void led_color_set(uint32_t color);

#endif // GPIO_H
