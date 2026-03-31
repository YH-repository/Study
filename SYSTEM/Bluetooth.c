#include "Bluetooth.h"



volatile char buf[BUF_SIZE];
volatile int buf_index = 0;
volatile uint8_t cmd_ready = 0;//接收完成标志

volatile uint32_t rx_timeout = 0;   // 超时计数

//pa2------tx usart2
//pa3------rx usart2
//外设需要使能中断
void USART2_Init()
{
    //使能时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    //TX PA2
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    //RX PA3
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    //配置串口
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600; //波特率 Hc-05默认
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; //每次发送8位
    USART_InitStructure.USART_StopBits = USART_StopBits_1; //没发完一帧，就停一个
    USART_InitStructure.USART_Parity = USART_Parity_No;//不做错误检测
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//不打断，不握手，直接说
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;//模式可收可发
    USART_Init(USART2,& USART_InitStructure);

    USART_Cmd(USART2,ENABLE);


    //配置NVIC
    NVIC_InitTypeDef NVIC_Initstructure;
    NVIC_Initstructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_Initstructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_Initstructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Initstructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_Initstructure);

    //使能USART中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);/*USART_IT_RXNE：接收寄存器非空（有新数据可读）
                                                - USART_IT_TXE：发送缓冲区空（可以发送下一个数据）
                                                - USART_IT_TC：发送完成*/
    NVIC_EnableIRQ(USART2_IRQn);   // 开启 USART2 中断,中断控制器

}

//发送字符
void USART2_SendChar(char c)
{
    USART_SendData(USART2, c);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);/*USART_FLAG_TXE（发送寄存器空）
                                                                USART_FLAG_TXE（发送寄存器空）
                                                                USART_FLAG_RXNE（接收寄存器非空）
                                                                返回值：RESET = 0 → 标志位没发生
                                                                        SET   = 1 → 标志位发生了*/
   
}
//发送字符串
void USART2_SendString(char *str)
{
    while(*str)
    {
        USART2_SendChar(*str++);
    }
}

//接收字符
// char USART2_ReceiveChar(void)
// {
//     while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET);
//     return USART_ReceiveData(USART2);
// }
//接收字符串
// 接收字符串（安全版），使用中断可以把此函数删除
// void USART2_ReceiveString(char *buf, int max_len)
// {
//     int i = 0;
//     char c;

//     while(1)
//     {
//         c = USART2_ReceiveChar();//会自动读后面一个字符

//         if(c == '\r' || c == '\n' || c== '\0' || c == ' ')//\r是回车\n是换行
//         {
//             if(i == 0)
//                 continue;//是跳出本次循环继续下一次循环

//             buf[i] = '\0';
//             break;//直接跳出循环
//         }

//         if(i < max_len - 1)
//         {
//             buf[i++] = c;
//         }
//     }
// }


//中断函数
void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2,USART_IT_RXNE) != RESET)
    {
        char c = USART_ReceiveData(USART2);

        if(buf_index < BUF_SIZE - 1)
        {
            buf[buf_index++] = c;
        }
        else
        {
            buf_index = 0;
        }
         // ⭐ 每收到一个字符 → 重置超时
        rx_timeout = 50;   // 50ms容易粘包改小一点
        // if(c == '\r' || c == '\n' || c == ' ')
        // {
        //     if(buf_index == 0)
        //         return;

        //     buf[buf_index] = '\0';
        //     buf_index = 0;
        //     cmd_ready = 1;//标志位表示已满
        // }
        // else
        // {
        //     if(buf_index < BUF_SIZE - 1)
        //     {
        //         buf[buf_index++] = c;
        //     }
        //     else
        //         buf_index = 0;
        // }
    }
}


