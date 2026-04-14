#include "stm32f10x.h"
#include "led.h"
#include "Motor.h"
#include "Delay.h"
#include "Mq2.h"
#include "fire.h"
#include "Bluetooth.h"
#include "oled.h"
#include <string.h>

// ================== FreeRTOS 头文件 ==================
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"



#define HIGH_THRESHOLD 2000
#define LOW_THRESHOLD  1900

static uint8_t smoke_alarm = 0;

//定义时间戳
uint32_t t_sensor = 0;
uint32_t t_control = 0;
uint32_t t_oled = 0;
uint32_t t_uart = 0;
uint8_t auto_flag = 0; // //0是自动，1是手动

uint16_t analog = 0;
uint16_t Fire = 1;
uint16_t digital = 1;


//全局时间
//volatile uint32_t ms_tick = 0;


int main()
{
    OLED_GPIO_Init();
    OLED_Init();
    MQ2_Init();
    SysTick_Init();
    motor_pwm_init();
    USART2_Init();
    Led_lib_init();

    USART2_SendString("system start\r\n");

    OLED_ShowChinese(0,0,"烟雾浓度：");
    OLED_ShowChinese(0,16,"火焰：");
    OLED_ShowChinese(0,32,"风扇状态：");
    OLED_ShowChinese(0,48,"灯状态：");
    
    OLED_Update();

    while(1)
    {
        uint32_t now = ms_tick;
      
         // ===== 1. 传感器任务（100ms）=====
        if(now - t_sensor >= 100)
        {
            t_sensor = now;

            uint32_t sum = 0;
            for(int i=0;i<5;i++)
            {
                sum += MQ2_Read_Analog();
            }
            analog = sum / 5;

            Fire = Fire_Read_Digital();
            digital = MQ2_Read_Digital();
        }

        // ===== 2. 控制任务（100ms）=====
        if(now - t_control >= 100)
        {
            t_control = now;

            if(analog > HIGH_THRESHOLD)
            {
                smoke_alarm = 1;
            }
            else if(analog < LOW_THRESHOLD)
            {
                smoke_alarm = 0;
            }

            
            if(smoke_alarm == 1 || digital == 0 || Fire == 0)
            {
                Led_lib_Ctrl(LED_ON);
                motor_forward();//正转
                auto_flag = 1;
                    //	delay_ms(5000);
            }
            else
            {
                if (auto_flag == 1)
                {
                    Led_lib_Ctrl(LED_OFF);
                    motor_stop();
                    auto_flag = 0;
                }   
            }
            
        }

         //===== 3. OLED任务（300ms）=====
        if(now - t_oled >= 300)
        {
            t_oled = now;

            if(Fire == 1)
            OLED_ShowChinese(48,16,"无火");
            else
                OLED_ShowChinese(48,16,"有火");
            if(motor_state == 0)
                OLED_ShowChinese(80,32,"关闭");
            else
                OLED_ShowChinese(80,32,"打开");
            if(light_state == 0)
                OLED_ShowChinese(64,48,"关闭");
            else
                OLED_ShowChinese(64,48,"打开");
            OLED_ShowNum(80,0,analog,4,OLED_8X16);
            OLED_UpdateArea(80,0,32,16);
            OLED_UpdateArea(48,16,32,16);
            OLED_UpdateArea(80,32,32,16);
            OLED_UpdateArea(64,48,32,16);
        }
        // ===== 4. 蓝牙发送任务（1秒）=====
        if(now - t_uart >= 1000)
        {
            t_uart = now;

            char buf_tx[32];
            sprintf(buf_tx,"<S:%d,F:%d,M:%d,L:%d>\r\n", analog, Fire, motor_state, light_state);

            USART2_SendString(buf_tx);
        }

        // ===== 5. 串口接收处理（随时）=====
        if(rx_timeout == 0 && buf_index > 0)
        {
            buf[buf_index] = '\0';//要手动补\0
            buf_index = 0;
            cmd_ready = 1;
        }
        if(cmd_ready)
        {
            
            cmd_ready = 0;

            if(strcmp((char *)buf,"onlight") == 0)
            {
                
                Led_lib_Ctrl(LED_ON);
                // USART2_SendString((char *)buf);
                auto_flag = 0;
                
                USART2_SendString("open success\r\n");
				USART2_SendString("please enter the command\r\n");			

            }
            else if(strcmp((char *)buf,"offlight") == 0)
            {
                
                Led_lib_Ctrl(LED_OFF);
                auto_flag = 0;
                
                USART2_SendString((char *)buf);
                USART2_SendString(" success\r\n");
                USART2_SendString("please enter the command1\r\n");
            }
            else if(strcmp((char *)buf,"onmotor") == 0)
            {
                
                motor_forward_pwm(1000); 
                //light_state = 1; 
                auto_flag = 0;
                USART2_SendString((char *)buf);
                USART2_SendString(" success\r\n");
                USART2_SendString("please enter the command1\r\n");
            }
            else if(strcmp((char *)buf,"offmotor") == 0)
            {
                
                motor_stop_pwm();
                //light_state = 0; 
                auto_flag = 0;
                USART2_SendString((char *)buf);
                USART2_SendString(" success\r\n");
                USART2_SendString("please enter the command1\r\n");
            }
            else
            {
                USART2_SendString((char *)buf);
                USART2_SendString(" send error!!\r\n");
                USART2_SendString("please enter the valid command\r\n");
            }
						
        }
    }
}










//正常运行
// int main()
// {
    


//     OLED_GPIO_Init();
//     OLED_Init();
//     MQ2_Init();
//     SysTick_Init();
//     motor_pwm_init();
//     USART2_Init();
//     Led_lib_init();

  
//     //uint16_t base = MQ2_Read_Analog();//获取初始环境的浓度
   
//     uint8_t auto_flag = 0; //0是自动，1是手动
//     USART2_SendString("please enter the command\r\n");

//     OLED_ShowChinese(0,0,"烟雾浓度：");
//     OLED_ShowChinese(0,16,"火焰：");
//     OLED_ShowChinese(0,32,"风扇状态：");
//     OLED_ShowChinese(0,48,"灯状态：");
    
//     OLED_Update();


//     while(1)
//     {
//        // OLED_Clear();
//         uint32_t sum = 0;
// 		//motor_forward_pwm(1000);
//         for(int i=0; i<5; i++) 
//         {
//             sum += MQ2_Read_Analog();
//         }
//         uint16_t analog = sum / 5;
//         uint8_t Fire = Fire_Read_Digital();//火焰有无，1--无火，0--有火
//         uint8_t digital = MQ2_Read_Digital();//数字值

//         if(Fire == 1)
//             OLED_ShowChinese(48,16,"无火");
//         else
//             OLED_ShowChinese(48,16,"有火");
//         if(motor_state == 0)
//             OLED_ShowChinese(80,32,"关闭");
//         else
//             OLED_ShowChinese(80,32,"打开");
//         if(light_state == 0)
//             OLED_ShowChinese(64,48,"关闭");
//         else
//             OLED_ShowChinese(64,48,"打开");
//         // OLED_ShowChinese(0,0,"烟雾浓度：");
//         // OLED_ShowChinese(0,16,"火焰：");
//         // OLED_ShowChinese(0,32,"风扇状态：");
//         // OLED_ShowChinese(0,48,"灯状态：");
//         OLED_ShowNum(80,0,analog,4,OLED_8X16);
//         OLED_UpdateArea(80,0,32,16);
//         OLED_UpdateArea(48,16,32,16);
//         OLED_UpdateArea(80,32,32,16);
//         OLED_UpdateArea(64,48,32,16);
//         //OLED_Update();
		
        
    
//         if(analog > HIGH_THRESHOLD)
//         {
//             smoke_alarm = 1;
//         }
//         else if(analog < LOW_THRESHOLD)
//         {
//             smoke_alarm = 0;
//         }

//         if(auto_flag == 0)
//         {
//             if(smoke_alarm == 1 || digital == 0 || Fire == 0)
//             {
//                 Led_lib_Ctrl(LED_ON);
//                 motor_forward();//正转
//                 auto_flag = 1;
//                     //	delay_ms(5000);
//             }
//             else
//             {
//                 if (auto_flag == 1)
//                 {
//                     Led_lib_Ctrl(LED_OFF);
//                     motor_stop();
//                     auto_flag = 0;
//                 }   
//             }
//         }
        
    
        
       
//        // static uint8_t send_cnt = 0;

//       //  send_cnt++;
//        // if(send_cnt >= 10)   // 控制发送频率（大约1秒一次）
//        // {

//            // send_cnt = 0;
//         delay_ms(1000);
//             char buf_tx[32];
//             sprintf(buf_tx,"<S:%d,F:%d,M:%d,L:%d>\r\n", analog, Fire, motor_state, light_state);

//             USART2_SendString(buf_tx);
//         //}
        
//         if(rx_timeout == 0 && buf_index > 0)
//         {
//             buf[buf_index] = '\0';//要手动补\0
//             buf_index = 0;
//             cmd_ready = 1;
//         }
//         if(cmd_ready)
//         {
            
//             cmd_ready = 0;

//             if(strcmp((char *)buf,"onlight") == 0)
//             {
                
//                 Led_lib_Ctrl(LED_ON);
//                 // USART2_SendString((char *)buf);
//                 auto_flag = 0;
                
//                 USART2_SendString("open success\r\n");
// 				USART2_SendString("please enter the command\r\n");			

//             }
//             else if(strcmp((char *)buf,"offlight") == 0)
//             {
                
//                 Led_lib_Ctrl(LED_OFF);
//                 auto_flag = 0;
                
//                 USART2_SendString((char *)buf);
//                 USART2_SendString(" success\r\n");
//                 USART2_SendString("please enter the command1\r\n");
//             }
//             else if(strcmp((char *)buf,"onmotor") == 0)
//             {
                
//                 motor_forward_pwm(1000); 
//                 //light_state = 1; 
//                 auto_flag = 0;
//                 USART2_SendString((char *)buf);
//                 USART2_SendString(" success\r\n");
//                 USART2_SendString("please enter the command1\r\n");
//             }
//             else if(strcmp((char *)buf,"offmotor") == 0)
//             {
                
//                 motor_stop_pwm();
//                 //light_state = 0; 
//                 auto_flag = 0;
//                 USART2_SendString((char *)buf);
//                 USART2_SendString(" success\r\n");
//                 USART2_SendString("please enter the command1\r\n");
//             }
//             else
//             {
//                 USART2_SendString((char *)buf);
//                 USART2_SendString(" send error!!\r\n");
//                 USART2_SendString("please enter the valid command\r\n");
//             }
						
//         }


//     }
// //    OLED_Clear();







//     // USART2_Init();
//     // Led_lib_init();
//     // SysTick_Init();
//     // //中断实现usart通信
//     // USART2_SendString("please enter the command\r\n");
//     // while(1)
//     // {
//     //    if(rx_timeout == 0 && buf_index > 0)
//     //    {
//     //         buf[buf_index] = '\0';//要手动补\0
//     //         buf_index = 0;
//     //         cmd_ready = 1;
//     //    }
//     //     if(cmd_ready)
//     //     {
            
//     //         cmd_ready = 0;

//     //         // //去掉末尾空格和回车
//     //         // int len = strlen((char *)buf);
//     //         // while(len > 0 && (buf[len - 1] == ' ' || buf[len - 1] == '\r' || buf[len - 1] == '\n'))
//     //         //     buf[--len] = '\0';

//     //         //把接收的数据发到手机上
//     //         //USART2_SendString((char *)buf);
//     //        // USART2_SendString("\r\n");

//     //         if(strcmp((char *)buf,"on") == 0)
//     //         {
//     //             Led_lib_Ctrl(LED_ON);
//     //            // USART2_SendString((char *)buf);
//     //             USART2_SendString("open success\r\n");
// 	// 			USART2_SendString("please enter the command\r\n");			

//     //         }
//     //         else if(strcmp((char *)buf,"off") == 0)
//     //         {
//     //             Led_lib_Ctrl(LED_OFF);
//     //             USART2_SendString((char *)buf);
//     //             USART2_SendString(" success\r\n");
//     //             USART2_SendString("please enter the command\r\n");
//     //         }
//     //         else
//     //         {
//     //             USART2_SendString((char *)buf);
//     //             USART2_SendString(" send error!!\r\n");
//     //             USART2_SendString("please enter the valid command\r\n");
//     //         }
						
//     //     }

//     // }
		 












//     //蓝牙测试
//     // USART2_Init();
//     // SysTick_Init();
//     // Led_lib_init();
//     // char buf[50];

//     // while(1)
//     // {
// 	// 		//简单的通过蓝牙模块控制灯
//     //     USART2_ReceiveString(buf,50);
//     //     USART2_SendString(buf);
//     //     USART2_SendString("\r\n");
//     //     if(strcmp(buf, "on") == 0)
//     //     {
//     //         Led_lib_Ctrl(LED_ON);
//     //     }
//     //     else if(strcmp(buf, "off") == 0)
//     //     {
//     //         Led_lib_Ctrl(LED_OFF);
//     //     }
//     // }



//     // Fire_Init();
//     // MQ2_Init();
//     // Led_lib_init();
//     // SysTick_Init();
//     // motor_init();
//     // //Led_lib_Ctrl(LED_OFF);
//     // while(1)
//     // {   
//     //     uint32_t sum = 0;
//     //     for(int i=0; i<5; i++) 
//     //     {
//     //         sum += MQ2_Read_Analog();
//     //     }
//     //     uint16_t analog = sum / 5; //模拟值
//     //     uint8_t Fire = Fire_Read_Digital();//火焰有无，1--无火，0--有火 
//     //     uint8_t digital = MQ2_Read_Digital();//数字值
//     //     if(analog > MQ2_ANALOG_THRESHOLD || digital == 0 || Fire == 0)
//     //     {
//     //         Led_lib_Ctrl(LED_ON);
//     //         motor_forward();//正转
// 	// 			//	delay_ms(5000);
//     //     }
//     //     else
//     //     {
//     //         Led_lib_Ctrl(LED_OFF);
//     //         motor_stop();
//     //     }
//     //     delay_ms(500);
//     // }


// 	//点灯
// 	// Led_lib_init();
// 	// SysTick_Init();
// 	// while(1)
// 	// {
// 	// 		Led_lib_Ctrl(LED_ON);
// 	// 		//delay_ms(2000);
// 	// 		delay_ms(1000);
// 	// 		Led_lib_Ctrl(LED_OFF);
// 	// 		delay_ms(1000);
// 	// 		//delay_ms(2000);
// 	// }


//     //控制电机驱动风扇
//    // motor_init();
//    //motor_pwm_init();
//     // while(1)
//     // {
//         // motor_forward();//正转
//         // delay(50000000);
//         // motor_backward();//反转
//         // delay(50000000);
//         // motor_stop();
//         // delay(50000000);

//         //使用pwm控制电机
//          //motor_forward_pwm(500);   // 50%占空比
//         // delay(50000000);
//         // motor_backward_pwm(500);
//         // delay(50000000);
//         // motor_stop_pwm();
//         // delay(50000000);
//     // }
  
// }

