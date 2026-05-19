#ifndef _MQ2_H_
#define _MQ2_H_
#include "stm32f10x.h"

extern volatile uint16_t analog;

void MQ2_NVIC_Init(void);
void MQ2_DMA_Init(void);

void MQ2_Init(void);
//uint16_t MQ2_Read_Analog(void);
uint8_t MQ2_Read_Digital(void);



#endif
