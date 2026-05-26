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
#include "queue.h"//必须放在freertos的上面
#include "IWDG.h"


#define HIGH_THRESHOLD 2000
#define LOW_THRESHOLD  1700


static uint8_t smoke_alarm = 0;

//裸机开发时使用
// uint32_t t_sensor = 0;
// uint32_t t_control = 0;
// uint32_t t_oled = 0;
// uint32_t t_uart = 0;
//uint8_t auto_flag = 0; // //0???????1?????

typedef enum {
    STATE_IDLE,          // 自动关闭
    STATE_AUTO_ACTIVE,   // 自动开启
    STATE_MANUAL         // 手动控制（临时）
} SystemStatus;

typedef enum {
    CMD_LIGHT_ON,
    CMD_MOTOR_ON,
    CMD_MOTOR_OFF,
    CMD_LIGHT_OFF
} ControlCmd_t;

// typedef enum
// {
//     STATE_IDLE,        // 空闲（手动控制）
//     STATE_AUTO_ACTIVE  // 自动控制中
// } SystemState;



/*
// ================== 控制意图 ==================
uint8_t manual_motor = 0;
uint8_t manual_light = 0;

uint8_t auto_motor = 0;
uint8_t auto_light = 0;
*/




//给采集任务使用
typedef struct
{
    uint16_t analog;
    uint16_t digital;
    uint8_t Fire;
} SensorData_t;

//使用结构体存储全局变量方便后续使用队列给oled任务和蓝牙任务使用
typedef struct
{
    uint16_t analog;
    uint8_t motor;
    uint8_t light;
    uint8_t Fire;
} SystemState_t;

//创建队列
QueueHandle_t xQueueSensor;
QueueHandle_t xQueueState;
//QueueHandle_t xQueueUART;
QueueHandle_t xQueueCmd;


//volatile uint16_t analog = 0;

// volatile uint16_t Fire = 1;
// volatile uint16_t digital = 1;


// // 重定向printf到串口2
// int fputc(int ch, FILE *f)
// {
// 	USART_SendData(USART2, (uint8_t)ch);
// 	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
// 	return ch;
// }


// ================== 任务句柄 ==================
TaskHandle_t xHandleSensor;
TaskHandle_t xHandleControl;
TaskHandle_t xHandleOLED;
TaskHandle_t xHandleUART;


/// ================== 任务1：传感器采集（100ms）==================
void vTaskSensor(void *pvParameters)
{
    SensorData_t data;
    while(1)
    {
        // uint32_t sum = 0;
        // for(int i=0;i<5;i++)
        // {
        //     sum += MQ2_Read_Analog();
        // }
        // analog = sum / 5;

        data.analog = analog; // DMA更新的
        data.digital = MQ2_Read_Digital();
        data.Fire = Fire_Read_Digital();

        //发送数据到队列，最后一个参数是最大等待时间（死等）
        xQueueSend(xQueueSensor, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
        
    }
}


// ================== 任务2：逻辑控制（100ms）==================
void vTaskControl(void *pvParameters)
{
    SensorData_t sensor;
    SystemState_t state;
    ControlCmd_t cmd;
    SystemStatus states = STATE_IDLE;
    //SystemState states = STATE_IDLE;
    while(1)
    {

        /*xQueueReceive 执行完会返回一个状态：
            pdPASS = 读取成功，拿到数据了
            errQUEUE_EMPTY = 读取失败，队列空的*/
        
        if(xQueueReceive(xQueueSensor, &sensor, portMAX_DELAY) == pdPASS)
        {
             IWDG_ReloadCounter();

            if(sensor.analog > HIGH_THRESHOLD)
            {
                smoke_alarm = 1;
                // auto_motor = 1;
                // auto_light = 1;
            }
            else if(sensor.analog < LOW_THRESHOLD)
            {
                smoke_alarm = 0;
                // auto_motor = 0;
                // auto_light = 0;
            }

            uint8_t danger = (smoke_alarm == 1) || 
                             (sensor.digital == 0) || 
                             (sensor.Fire == 0);
            if(xQueueReceive(xQueueCmd, &cmd, 0) == pdPASS)
            {
                switch(cmd)
                {
                    case CMD_MOTOR_ON:
                        states = STATE_MANUAL;
                        motor_forward_pwm(1000); 
                        break;

                    case CMD_MOTOR_OFF:
                        states = STATE_MANUAL;
                        motor_stop_pwm();
                        break;

                    case CMD_LIGHT_ON:  
                        Led_lib_Ctrl(LED_ON); 
                        states = STATE_MANUAL;
                        break;

                    case CMD_LIGHT_OFF:
                        Led_lib_Ctrl(LED_OFF);
                        states = STATE_MANUAL;
                        break;
                }
            }
            switch(states)
            {
                case STATE_IDLE:
                {
                    Led_lib_Ctrl(LED_OFF);
                    motor_stop();
                    if(danger)
                    {
                        states = STATE_AUTO_ACTIVE;
                    }
                    break;
                }
                case STATE_AUTO_ACTIVE:
                {
                    Led_lib_Ctrl(LED_ON);
                    motor_forward();//???
                    if(!danger)
                    {
                        states = STATE_IDLE;
                    }
                    break;
                }

                case STATE_MANUAL:
                {
                    // ? 关键点：自动可以“抢回控制权”
                    if(danger)
                    {
                        states = STATE_AUTO_ACTIVE;   // 覆盖手动
                    }
                    break;
                }
                default:
                    states = STATE_IDLE;
                    break;
            }
            

            // if(danger)
            // {
            //     Led_lib_Ctrl(LED_ON);
            //     motor_forward();//???
            //     states = STATE_AUTO_ACTIVE;
            //         //	delay_ms(5000);
            // }
            // else
            // {
            //     if (states == STATE_AUTO_ACTIVE)
            //     {
            //         Led_lib_Ctrl(LED_OFF);
            //         motor_stop();
            //         states = STATE_IDLE;
            //     }   
            // }
            //更新数据
            state.analog = sensor.analog;
            state.motor = motor_state;
            state.light = light_state;
            state.Fire = sensor.Fire;
            //再发送到队列
            xQueueOverwrite(xQueueState, &state); 
        }
        

        //vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// ================== 任务3：OLED刷新（300ms）==================
void vTaskOLED(void *pvParameters)
{
    SystemState_t state;

    while(1)
    {
        if(xQueuePeek(xQueueState,&state,portMAX_DELAY) == pdPASS)
        {
            if(state.Fire == 1)
                OLED_ShowChinese(48,16,"无火");
            else
                OLED_ShowChinese(48,16,"有火");
            if(state.motor == 0)
                OLED_ShowChinese(80,32,"关闭");
            else
                OLED_ShowChinese(80,32,"打开");
            if(state.light == 0)
                OLED_ShowChinese(64,48,"关闭");
            else
                OLED_ShowChinese(64,48,"打开");
            OLED_ShowNum(80,0,state.analog,4,OLED_8X16);
            // 必须用全屏刷新，不要用 UpdateArea！！
            //OLED_Update();
            
            OLED_UpdateArea(80,0,32,16);
            OLED_UpdateArea(48,16,32,16);
            OLED_UpdateArea(80,32,32,16);
            OLED_UpdateArea(64,48,32,16);
        }
        
    
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}


// ================== 任务4：蓝牙发送 + 接收解析（10ms）==================
void vTaskUART(void *pvParameters)
{
    SystemState_t state;
    ControlCmd_t cmd;
    while(1)
    {
        //1秒发一次
        static uint8_t cnt = 0;
        
        //获取最新状态,Peek：复制了数据但是仍留下一个副本
        xQueuePeek(xQueueState,&state,0);

        if(cnt++ >= 100)
        {
            cnt = 0;
            char buf_tx[32];
            sprintf(buf_tx,"<S:%d,F:%d,M:%d,L:%d>\r\n", state.analog, state.Fire, state.motor, state.light);
            USART2_SendString(buf_tx);
        }

      
        //蓝牙指令处理
       
        
        if(cmd_ready)
        {
            
            cmd_ready = 0;
            // ?补结束符
            rx_buffer[rx_len] = '\0';
            // buf[buf_index] = '\0';
            // for(int i = 0; i < buf_index; i++)
            // {
            //     if(buf[i] == '\r' || buf[i] == '\n')
            //     {
            //         buf[i] = '\0';
            //     }
            // }
            // buf_index = 0;

            if(strstr((char *)rx_buffer,"onlight") != NULL)
            {
               //OLED_ShowString(0,0,"ENTER ONLIGHT",16); // 测试用
                
              //  Led_lib_Ctrl(LED_ON);
                // USART2_SendString((char *)buf);
                //auto_flag = 0;
                
                USART2_SendString("open success\r\n");
				USART2_SendString("please enter the command\r\n");			
                cmd = CMD_LIGHT_ON;
                xQueueSend(xQueueCmd, &cmd, 0);
            }
            else if(strstr((char *)rx_buffer,"offlight") != NULL)
            {
                
                //Led_lib_Ctrl(LED_OFF);
                //auto_flag = 0;
                
                USART2_SendString((char *)rx_buffer);
                USART2_SendString(" success\r\n");
                USART2_SendString("please enter the command1\r\n");
                cmd = CMD_LIGHT_OFF;
                xQueueSend(xQueueCmd, &cmd, 0);
            }
            else if(strstr((char *)rx_buffer,"onmotor") != NULL)
            {
                
                //motor_forward_pwm(1000); 
                //light_state = 1; 
                //auto_flag = 0;
                USART2_SendString((char *)rx_buffer);
                USART2_SendString(" success\r\n");
                USART2_SendString("please enter the command1\r\n");
                cmd = CMD_MOTOR_ON;
                xQueueSend(xQueueCmd, &cmd, 0);
            }
            else if(strstr((char *)rx_buffer,"offmotor") != NULL)
            {
                
                //motor_stop_pwm();
                //light_state = 0; 
                //auto_flag = 0;
                USART2_SendString((char *)rx_buffer);
                USART2_SendString(" success\r\n");
                USART2_SendString("please enter the command1\r\n");
                cmd = CMD_MOTOR_OFF;
                xQueueSend(xQueueCmd, &cmd, 0);
            }
            else
            {
                USART2_SendString((char *)rx_buffer);
                USART2_SendString(" send error!!\r\n");
                USART2_SendString("please enter the valid command\r\n");
            }
			 			
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}




int main()
{

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    OLED_GPIO_Init();
    OLED_Init();
    MQ2_Init();
  
    MQ2_NVIC_Init();
    motor_pwm_init();
    USART2_Init();
    Led_lib_init();
    IWDG_Init();

    USART2_SendString("system start\r\n");

    OLED_ShowChinese(0,0,"烟雾浓度：");
    OLED_ShowChinese(0,16,"火焰：");
    OLED_ShowChinese(0,32,"风扇状态：");
    OLED_ShowChinese(0,48,"灯状态：");
    
    OLED_Update();
    
    //创建队列要在main函数中
    //队列长度：5，每个元素：SensorData_t
    xQueueSensor = xQueueCreate(5, sizeof(SensorData_t));
    //用 overwrite,xQueueState 用 长度=1（关键！）是为了获取的是最新的数据
    xQueueState  = xQueueCreate(1, sizeof(SystemState_t)); 
    xQueueCmd = xQueueCreate(10, sizeof(ControlCmd_t)); 
    //xQueueUART   = xQueueCreate(1, sizeof(SystemState_t));


    // ================== 创建 FreeRTOS 任务 ==================
    xTaskCreate(vTaskSensor,  "Sensor",  512, NULL, 2, &xHandleSensor);
    xTaskCreate(vTaskControl, "Control", 512, NULL, 3, &xHandleControl);
    xTaskCreate(vTaskOLED,    "OLED",    512, NULL, 1, &xHandleOLED);
    xTaskCreate(vTaskUART,    "UART",    512, NULL, 4, &xHandleUART);
   

    // 启动调度器
    vTaskStartScheduler();

    while(1);
}





//??????
//volatile uint32_t ms_tick = 0;

/*
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

         // ===== 3. OLED任务（300ms）=====
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

*/






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
