#ifndef __LED_LIB_H_
#define __LED_LIB_H_
#include "stm32f10x.h"

extern uint16_t light_state;
//pc13鎺у埗鐏?浜?
#define LED_ON 0
#define LED_OFF 1 
#define LED_ON 0
#define LED_OFF 1 
void Led_lib_init(void);
void Led_lib_Ctrl(int led_state);
void delay(int time);

#endif
