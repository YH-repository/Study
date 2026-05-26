#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "queue.h"//必须放在freertos的下面
#include <string.h>

#define RX_BUFFER_SIZE 100
//#define BUF_SIZE 50
#define FRAME_HEAD '<'
#define FRAME_TAIL '>'

extern QueueHandle_t xQueueCmd;

typedef enum {
    CMD_NONE,
    CMD_LIGHT_ON,
    CMD_MOTOR_ON,
    CMD_MOTOR_OFF,
    CMD_LIGHT_OFF
} ControlCmd_t;

extern uint8_t rx_buffer[RX_BUFFER_SIZE];   // DMA接收缓冲区
extern uint16_t rx_len;

extern volatile uint8_t cmd_ready;
//extern volatile char buf[BUF_SIZE];
//extern volatile uint32_t rx_timeout;   // 超时计数
//extern volatile int buf_index;

void Parse_Frame(uint8_t *buf, uint16_t len);
void UART_DMA_Init(void);

void USART2_Init(void);
void USART2_SendChar(char c);
void USART2_SendString(char *str);

//char USART2_ReceiveChar(void);
//void USART2_ReceiveString(char *buf, int max_len);

#endif
