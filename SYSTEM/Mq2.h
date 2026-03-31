#ifndef _MQ2_H_
#define _MQ2_H_
#include "stm32f10x.h"

void MQ2_Init(void);
uint16_t MQ2_Read_Analog(void);
uint8_t MQ2_Read_Digital(void);



#endif
