#include "stm32f4xx.h"

/*定义所需的初始化结构体变量*/
GPIO_InitTypeDef GPIO_InitStructure;

u16 Adc = 0;           // ADC值
u16 Blink_Delay = 0;   // 闪烁间隔(ms)

/* 函数声明 */
void delay_us(uint16_t nus);
void delay_ms(u16 nms);
void Adc_Init(void);
uint16_t Get_Adc(uint8_t ch);
uint16_t Get_Adc_Average(uint8_t ch, uint8_t times);

int main(void)
{
    /* ---- 配置 LED1（PB5）为普通 GPIO 输出 ---- */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_SetBits(GPIOB, GPIO_Pin_5);      // 初始熄灭
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* ---- 初始化 ADC（PC1, Channel 11）---- */
    Adc_Init();

    while(1)
    {
        /* 读取 ADC 10 次取平均值 */
        Adc = Get_Adc_Average(ADC_Channel_11, 10);

        /*
         * ADC 范围 0~4095 映射到延时 100ms~2000ms
         * delay = 100 + (Adc * 1900) / 4095
         * 电位器最小 → 100ms（快闪）
         * 电位器最大 → 2000ms（慢闪）
         */
        Blink_Delay = 100 + (Adc * 1900) / 4095;

        /* 翻转 LED：亮 → 灭 → 亮 → 灭 ... */
        GPIO_ResetBits(GPIOB, GPIO_Pin_5);   // 点亮 LED1
        delay_ms(Blink_Delay);               // 亮 Blink_Delay ms

        GPIO_SetBits(GPIOB, GPIO_Pin_5);     // 熄灭 LED1
        delay_ms(Blink_Delay);               // 灭 Blink_Delay ms
    }
}

/***** 简单延时函数 *****/
void delay_us(uint16_t nus)
{
    uint16_t i;
    while(nus--)
    {
        i = 31;
        while(i--);
    }
}

void delay_ms(u16 nms)
{
    uint16_t i;
    while(nms--)
    {
        i = 33800;
        while(i--);
    }
}

/***** ADC 初始化（PC1, ADC1 Channel 11）*****/
void Adc_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    /* PC1 配置为模拟输入 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* ADC 复位 */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);

    /* 通用设置：独立模式, 14MHz */
    ADC_CommonInitStructure.ADC_Mode             = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInitStructure.ADC_DMAAccessMode    = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_Prescaler        = ADC_Prescaler_Div6;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* 单次转换, 12bit, 右对齐 */
    ADC_InitStructure.ADC_Resolution           = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode         = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode   = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign            = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion      = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);
}

/***** 单次 ADC 转换 *****/
uint16_t Get_Adc(u8 ch)
{
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles);
    ADC_SoftwareStartConv(ADC1);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}

/***** 多次 ADC 转换取平均 *****/
uint16_t Get_Adc_Average(uint8_t ch, uint8_t times)
{
    uint32_t temp_val = 0;
    uint8_t i;
    for(i = 0; i < times; i++)
    {
        temp_val += Get_Adc(ch);
        delay_ms(5);
    }
    return temp_val / times;
}