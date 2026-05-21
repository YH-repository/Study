#include "IWDG.h"

#include "stm32f10x_iwdg.h"
//看闷狗在LSI内部低速时钟 RC 40 kHz
void IWDG_Init(void)
{
    //先使能
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    //分频器是由系统硬件预设的，只能固定分频比，不能随意设置 40000/64 = 625
    IWDG_SetPrescaler(IWDG_Prescaler_64);

    //重载值625
    IWDG_SetReload(625);

     // 4. 先喂一次狗
    IWDG_ReloadCounter();

    // 5. 启动看门狗
    IWDG_Enable();


}
