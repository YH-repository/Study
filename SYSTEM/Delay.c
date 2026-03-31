#include "Delay.h"

//中断实现
volatile uint32_t ms_tick = 0; // 毫秒计数

 


//内核不需要使能中断

void SysTick_Init(void)
{
		SysTick->LOAD = 72000 - 1;     // 72MHz → 1us = 72个周期从多少开始倒数相当于n值
		SysTick->VAL = 0;            // 清空计数器即初值从哪里开始
		SysTick->CTRL = 0x7;         //0b101 使能 + 使用内核时钟，bit0 —— ENABLE（使能位），
																	//bit1 —— TICKINT（中断开关），bit1 = 1 → 计数到0会触发中断,bit1 = 0 → 不触发中断
                                      //bit2 = 1 → 使用 CPU时钟（72MHz）bit2 = 0 → 使用 外部时钟（一般是72MHz/8）	
}

// //定时1us
// void delay_us1(void)
// {
// 		SysTick->LOAD = 72 - 1;     // 72MHz → 1us = 72个周期从多少开始倒数相当于n值
// 		SysTick->VAL = 0;            // 清空计数器即初值从哪里开始
// 		SysTick->CTRL = 0x7;         //0b101 使能 + 使用内核时钟，bit0 —— ENABLE（使能位），
// 																//bit1 —— TICKINT（中断开关），bit1 = 1 → 计数到0会触发中断,bit1 = 0 → 不触发中断
// 																//bit2 = 1 → 使用 CPU时钟（72MHz）bit2 = 0 → 使用 外部时钟（一般是72MHz/8） 
// }



//使用ucous不需要
void delay_ms(uint32_t ms)
{
	uint32_t start = ms_tick;            // 记录开始时间
    while((ms_tick - start) < ms)
    {
        // CPU 可以执行其他任务，或者空循环
    }
}




//us最好不用中断，因为太快了会影响cpu调用
// void delay_us2(uint32_t us)
// {
// 	uint32_t start = ms_tick;            // 记录开始时间
//     while((ms_tick - start) < us)
//     {
//         // CPU 可以执行其他任务，或者空循环
//     }
// }



//定时1ms
// void delay_ms(uint32_t ms)
// {
//     for(uint32_t i=0;i<ms;i++)
//     {
//         SysTick->LOAD = 72000 - 1;     // 72MHz → 1us = 72个周期从多少开始倒数相当于n值
//         SysTick->VAL = 0;            // 清空计数器即初值从哪里开始
//         SysTick->CTRL = 0x5;         //0b101 使能 + 使用内核时钟，bit0 —— ENABLE（使能位），
//                                     //bit1 —— TICKINT（中断开关），bit1 = 1 → 计数到0会触发中断,bit1 = 0 → 不触发中断
//                                       //bit2 = 1 → 使用 CPU时钟（72MHz）bit2 = 0 → 使用 外部时钟（一般是72MHz/8）
//         while(!(SysTick->CTRL & (1 << 16))); // 等待计数结束,CTRL 寄存器第16位（COUNTFLAG）,即倒数完毕后此值变为1
//         SysTick->CTRL = 0;           // 关闭
//     }
   
// }

//定时1us,使用ucous不需要
// void delay_us(uint32_t us)
// {
//     for(uint32_t i=0;i<us;i++)
//     {
//         SysTick->LOAD = 72 - 1;     // 72MHz → 1us = 72个周期从多少开始倒数相当于n值
//         SysTick->VAL = 0;            // 清空计数器即初值从哪里开始
//         SysTick->CTRL = 0x5;         //0b101 使能 + 使用内核时钟，bit0 —— ENABLE（使能位），
//                                     //bit1 —— TICKINT（中断开关），bit1 = 1 → 计数到0会触发中断,bit1 = 0 → 不触发中断
//                                     //bit2 = 1 → 使用 CPU时钟（72MHz）bit2 = 0 → 使用 外部时钟（一般是72MHz/8）

//         while(!(SysTick->CTRL & (1 << 16))); // 等待计数结束,CTRL 寄存器第16位（COUNTFLAG）,即倒数完毕后此值变为1

//         SysTick->CTRL = 0;           // 关闭
//     }
 
// }

