#ifndef _DELAY_H_
#define _DELAY_H_
#include "stm32f10x.h"

extern volatile uint32_t ms_tick;

void SysTick_Init(void);
//void delay_us1(void);

void delay_ms(uint32_t ms);

//void delay_us2(uint32_t us);

//void delay_ms(uint32_t ms);


//void delay_us(uint32_t us);

#endif

