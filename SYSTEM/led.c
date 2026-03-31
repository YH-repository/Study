#include "led.h"
#include "stm32f10x_rcc.h"
uint16_t light_state = 0;
/*正常的点灯程序 */
void Led_lib_init(void)
{
    //使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    //初始化GPIO
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;       
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
    
    GPIO_Init(GPIOC,&GPIO_InitStruct);

    GPIO_SetBits(GPIOC,GPIO_Pin_13);
}

void Led_lib_Ctrl(int led_state)
{
    switch(led_state)
    {
        case LED_ON:
            GPIO_ResetBits(GPIOC,GPIO_Pin_13);
            light_state = 1;
            break;

        case LED_OFF:
            GPIO_SetBits(GPIOC,GPIO_Pin_13);
            light_state = 0;
            break;

        default:
            break;
    }
}

