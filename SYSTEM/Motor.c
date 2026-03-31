#include "Motor.h"

#include "stm32f10x.h"
//L298N
//红 outb  黑 outa 
//pwm要有定时器通道才能使用
//in1-----pb0----- TIM3_CH3
//in2-----pb1----- TIM3_CH4
uint8_t motor_state = 0;
/*使用pwm实现*/
void motor_pwm_init()
{
    //使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);

    //GPIO复用推挽输出
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;       
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStruct);


    //定时器配置
    TIM_TimeBaseInitTypeDef TIM_BaseStruct;
    TIM_BaseStruct.TIM_Period = 999;//N值，相当于1ms一个周期
    TIM_BaseStruct.TIM_Prescaler = 71;//预分值
    TIM_BaseStruct.TIM_ClockDivision = 0;
    TIM_BaseStruct.TIM_CounterMode = TIM_CounterMode_Up;//向上计数
    TIM_TimeBaseInit(TIM3, &TIM_BaseStruct);

    //pwm模式配置
    TIM_OCInitTypeDef TIM_OCStruct;

    //ch3 ----pb0
    TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;//pwm模式小于限定值有效大于限定值无效
    TIM_OCStruct.TIM_OutputState = TIM_OutputNState_Enable;//相当于使能
    TIM_OCStruct.TIM_Pulse = 0;//就是那个比较值，占空比计算TIM_Pulse /（TIM_Period + 1）
    TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_High;//高电平有效就是上面有效处输入高电平
    TIM_OC3Init(TIM3, &TIM_OCStruct);

    //ch4-----pb1;
    TIM_OCStruct.TIM_Pulse = 0;
    TIM_OC4Init(TIM3, &TIM_OCStruct);

    // 5. 启动定时器
    TIM_Cmd(TIM3, ENABLE);
}
//设置占空比
void PWM_SetCompare3(uint16_t value)  // PB0
{
    TIM_SetCompare3(TIM3, value);//用来更改这个定时器的比较值
}

void PWM_SetCompare4(uint16_t value)  // PB1
{
    TIM_SetCompare4(TIM3, value);//用来更改这个定时器的比较值
}
//正转
void motor_forward_pwm(uint16_t speed)
{
    PWM_SetCompare3(speed);  // PB0 输出PWM
    PWM_SetCompare4(0);      // PB1 关闭
    motor_state = 1;
}
//反转
void motor_backward_pwm(uint16_t speed)
{
    PWM_SetCompare3(0);  // PB0 输出PWM
    PWM_SetCompare4(speed);      // PB1 关闭
    motor_state = 2;
}
//停止
void motor_stop_pwm(void)
{
    PWM_SetCompare3(0);
    PWM_SetCompare4(0);
    motor_state = 0;
}



/*正常的操作*/
void motor_init()
{
    //使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    //初始化GPIO
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;       
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
    
    GPIO_Init(GPIOB,&GPIO_InitStruct);
}
void motor_forward()
{
    GPIO_SetBits(GPIOB,GPIO_Pin_0);
    GPIO_ResetBits(GPIOB,GPIO_Pin_1);
    motor_state = 1;
}
void motor_backward()
{
    GPIO_ResetBits(GPIOB,GPIO_Pin_0);
    GPIO_SetBits(GPIOB,GPIO_Pin_1);
    motor_state = 2;
}
void motor_stop()
{
    GPIO_ResetBits(GPIOB,GPIO_Pin_0|GPIO_Pin_1);
    motor_state = 0;
}
