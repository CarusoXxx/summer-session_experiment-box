#include "stm32f4xx.h"

u16 DacValue = 0xFFF;//定义DAC的输出值，从最大值4095(3.3V)开始递减

/* 定义所需结构体*/
GPIO_InitTypeDef GPIO_InitStructure;//定义 GPIO 初始化结构体
DAC_InitTypeDef DAC_InitType; //定义 DAC 初始化结构体

/*函数声明*/
void delay_us(uint16_t nus);
void delay_ms(u16 nms);
void Adc_Init(void); //ADC初始化函数声明
uint16_t Get_Adc(uint8_t ch); //ADC转换一次的函数声明
uint16_t Get_Adc_Average(uint8_t ch,uint8_t times);//ADC转换多次的函数声明
int main(void)
{
RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//开启GPIOA时钟
RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);//开启DAC时钟

/* 配置PA4为DAC的输出端 */
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; //选择 GPIOA 的 Pin4
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; //模拟模式（输出）
GPIO_Init(GPIOA, &GPIO_InitStructure); //按照上述参数初始化GPIOA

/*配置DAC的输出参数*/
DAC_InitType.DAC_Trigger = DAC_Trigger_None; //不使用触发功能
DAC_InitType.DAC_WaveGeneration = DAC_WaveGeneration_None; //不用波形发生器
DAC_InitType.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;//屏蔽幅值
DAC_InitType.DAC_OutputBuffer = DAC_OutputBuffer_Disable; //关闭DAC1输出缓存
DAC_Init(DAC_Channel_1,&DAC_InitType); //初始化DAC通道1

DAC_Cmd(DAC_Channel_1, ENABLE); //使能DAC通道1

DAC_SetChannel1Data(DAC_Align_12b_R, 0xFFF); //12位右对齐，初始输出4095(约3.3V)
while(1)
{
    DAC_SetChannel1Data(DAC_Align_12b_R, DacValue);//从3.3V电压开始递减输出（PA4）

    /* 递减前先判界，避免 u16 下溢后数值超出 0~4095 */
    if(DacValue >= 248)
    {
        DacValue -= 248; //每次减少248，输出电压大约下降0.2V
    }
    else
    {
        DacValue = 0xFFF; //降到接近0后，重新从4095(3.3V)开始
    }

    delay_ms(1000);//每次延时1秒一直循环
}
}
/***** 简单延时函数,ms,us *****/
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
void Adc_Init(void)
{
GPIO_InitTypeDef GPIO_InitStructure; //定义GPIO初始化结构体
ADC_CommonInitTypeDef ADC_CommonInitStructure; //定义ADCCommon结构体
ADC_InitTypeDef ADC_InitStructure; //定义ADC初始化结构体

RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能AHB1上的GPIOC时钟
RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); //使能APB2上的ADC1时钟

GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC 1
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; //配置为模拟模式
GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ; //没有上下拉电阻
GPIO_Init(GPIOC, &GPIO_InitStructure); //初始化PC1

RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,ENABLE); //ADC复位
RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,DISABLE); //ADC复位结束


ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent; //ADC设置为独立模式
ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles; //2采样之间5个时钟
ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //不使用DMA
ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div6; //预分频=6 PCLK2/6 = 84/6 = 14Mhz
ADC_CommonInit(&ADC_CommonInitStructure);

ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; //12bit模式
ADC_InitStructure.ADC_ScanConvMode = DISABLE; //非扫描模式
ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; //关闭连续转换
ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; //右对齐
ADC_InitStructure.ADC_NbrOfConversion = 1; //转换序列1
ADC_Init(ADC1, &ADC_InitStructure);

ADC_Cmd(ADC1, ENABLE); //ADC开启
}
uint16_t Get_Adc(u8 ch) //ADC转换一次，480周期
{
/* 配置ADC转换参数 */
ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles );

ADC_SoftwareStartConv(ADC1); //开启ADC转换

while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC )); //等待ADC转换完成

return ADC_GetConversionValue(ADC1); //返回ADC转换数据
}
/* 应当转换多次数据取平均值才精确 */
/* 转换次数times一般不少于8次 */
uint16_t Get_Adc_Average(uint8_t ch,uint8_t times)
{
uint32_t temp_val=0;//定义一个32位变量用于累加多次的16位数据
uint8_t i;//循环变量
for(i = 0;i < times; i++)//循环times次数，每次转换一个ADC数据
{
temp_val+=Get_Adc(ch);//转换（采集）ADC数据
delay_ms(5); //延时一小会
}
return temp_val/times;//返回平均值：累加值 / 次数
}