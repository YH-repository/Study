#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include "stm32f10x.h"

#define BUF_SIZE 50

extern volatile uint8_t cmd_ready;
extern volatile char buf[BUF_SIZE];
extern volatile uint32_t rx_timeout;   // 超时计数
extern volatile int buf_index;

void USART2_Init(void);
void USART2_SendChar(char c);
void USART2_SendString(char *str);
//char USART2_ReceiveChar(void);
//void USART2_ReceiveString(char *buf, int max_len);

#endif
