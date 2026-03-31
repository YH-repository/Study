#include "Mq2.h"
/*
    GPIO_Mode_AIN：模拟输入adc
-   GPIO_Mode_IN_FLOATING：浮空输入
-    GPIO_Mode_IPD：下拉输入
-   GPIO_Mode_IPU：上拉输入
-   GPIO_Mode_Out_OD：开漏输出
-   GPIO_Mode_Out_PP：推挽输出
-   GPIO_Mode_AF_OD：复用开漏
-   GPIO_Mode_AF_PP：复用推挽
*/
//ad ---- pa7----ADC_IN7
//do ----pa6
void MQ2_Init()
{
    //使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1,ENABLE);
    //pa7模拟输入
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    //pa6数字输入,要上拉输入保证空闲时为高电平
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //adc初始化
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;/*ADC_Mode_Independent：独立模式
                                                        ADC_DualMode_RegSimult：两个 ADC 同时扫描（普通通道）
                                                        ADC_DualMode_RegAlterTrig：两个 ADC 交替触发扫描
                                                        ADC_DualMode_Interl：两个 ADC 交替采样（交错模式）*/
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;//ENABLE：扫描模式，ADC 会按配置顺序依次采样多个通道
                                                //DISABLE：单通道模式，只采样一个通道
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;//ENABLE：连续采样，采完一个通道后自动开始下一次采样
                                                    //DISABLE：单次采样，采完一次需要手动或触发再次采样
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;/*ADC_ExternalTrigConv_T1_CC1：定时器1的通道1事件触发
                                                                        ADC_ExternalTrigConv_T1_CC2：定时器1的通道2事件触发
                                                                        ADC_ExternalTrigConv_None：没有外部触发，用软件触发*/
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;/*ADC_DataAlign_Right：右对齐（默认），低位对齐，方便直接使用 12 位原始值
                                                            ADC_DataAlign_Left：左对齐，高位对齐，方便移位得到 8 位结果*/   
    ADC_InitStructure.ADC_NbrOfChannel = 1;//此处看要扫描多少个通道
    ADC_Init(ADC1, &ADC_InitStructure);
    //使能ADC1外设
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);//复位ADC校准寄存器
    while(ADC_GetResetCalibrationStatus(ADC1));//等待复位校准完成
    ADC_StartCalibration(ADC1);//启动自校准
    while(ADC_GetCalibrationStatus(ADC1));//等待校准完毕

    ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 1, ADC_SampleTime_55Cycles5);/*ADC1 → 使用 ADC1
                                                                                ADC_Channel_7 → 选择通道 7（对应 PA7）
                                                                                1 → 序列顺序号（第 1 个采样）
                                                                                ADC_SampleTime_55Cycles5 → 采样时间 55.5 个 ADC 时钟周期*/
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

// 读取模拟值（0~4095）
uint16_t MQ2_Read_Analog(void)
{
    return ADC_GetConversionValue(ADC1);
}

// 读取数字值
uint8_t MQ2_Read_Digital(void)
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6);
}

