#include "fire.h"

//do-----pa5
void Fire_Init()
{
    //使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    
}
uint8_t Fire_Read_Digital()
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5);
}

