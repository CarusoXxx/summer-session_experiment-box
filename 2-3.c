#include "stm32f4xx.h"

/* 结构体变量 */
GPIO_InitTypeDef  GPIO_InitStructure;
NVIC_InitTypeDef  NVIC_InitStructure;
EXTI_InitTypeDef  EXTI_InitStructure;

/* 时间变量 —— 加 volatile！ */
volatile u16 LedTime = 500;

/* 函数声明 */
void delay_us(uint16_t nus);
void delay_ms(u16 nms);

/***** 延时函数 *****/
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

/***** 中断服务函数 *****/
void EXTI0_IRQHandler(void)
{
    EXTI_ClearITPendingBit(EXTI_Line0);
    LedTime = 100;    // 5倍速
}

/***** 主函数 *****/
int main(void)
{
    /* ========== 1. 初始化LED ========== */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* ========== 2. 初始化外部中断（PA0按键）========== */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* ========== 3. LED流水灯死循环 ========== */
    while(1)
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_5);
        GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1);
        delay_ms(LedTime);

        GPIO_ResetBits(GPIOB, GPIO_Pin_0);
        GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_1);
        delay_ms(LedTime);

        GPIO_ResetBits(GPIOB, GPIO_Pin_1);
        GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0);
        delay_ms(LedTime);
    }
}