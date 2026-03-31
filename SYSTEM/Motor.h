#ifndef _MOTOR_H_
#define _MOTOR_H_


#include "stm32f10x.h"
 
extern uint8_t motor_state;
//pwm控制电机
void motor_pwm_init(void);
void PWM_SetCompare3(uint16_t value);
void PWM_SetCompare4(uint16_t value);
void motor_forward_pwm(uint16_t speed);
void motor_backward_pwm(uint16_t speed);
void motor_stop_pwm(void);
//控制电机
void motor_init(void);
void motor_forward(void);
void motor_backward(void);
void motor_stop(void);

#endif
