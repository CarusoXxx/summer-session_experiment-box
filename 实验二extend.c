#include "stm32f4xx.h"

GPIO_InitTypeDef GPIO_InitStructure; //定义一个GPIO初始化用的结构体

// ==================== 引脚定义 ====================
// LED1~LED3 接在 GPIOB
#define LED1_PIN    GPIO_Pin_5
#define LED2_PIN    GPIO_Pin_0
#define LED3_PIN    GPIO_Pin_1
#define LED_GPIO    GPIOB

// 蜂鸣器接在 PA8
#define BUZZER_PIN  GPIO_Pin_8
#define BUZZER_GPIO GPIOA

// KEY1=PA0, KEY2=PC13, KEY3=PC6, KEY4=PC7
#define KEY1_PIN    GPIO_Pin_0
#define KEY2_PIN    GPIO_Pin_13
#define KEY3_PIN    GPIO_Pin_6
#define KEY4_PIN    GPIO_Pin_7
#define KEY1_GPIO   GPIOA
#define KEY234_GPIO GPIOC

void delay_us(uint16_t nus)
{
   uint16_t i;
   while(nus--)
   {
       i = 31; 
       while(i--);
   }
}

void delay_ms(uint16_t nms)
{
   uint16_t i;
   while(nms--)
   {
       i = 33800;
       while(i--);
   }
}

// ==================== LED / 蜂鸣器基础操作 ====================
// LED 低电平点亮，高电平熄灭
void LED_All_On(void)
{
    GPIO_ResetBits(LED_GPIO, LED1_PIN | LED2_PIN | LED3_PIN);
}

void LED_All_Off(void)
{
    GPIO_SetBits(LED_GPIO, LED1_PIN | LED2_PIN | LED3_PIN);
}

void LED1_On(void)  { GPIO_ResetBits(LED_GPIO, LED1_PIN); }
void LED2_On(void)  { GPIO_ResetBits(LED_GPIO, LED2_PIN); }
void LED3_On(void)  { GPIO_ResetBits(LED_GPIO, LED3_PIN); }

void LED1_Off(void) { GPIO_SetBits(LED_GPIO, LED1_PIN); }
void LED2_Off(void) { GPIO_SetBits(LED_GPIO, LED2_PIN); }
void LED3_Off(void) { GPIO_SetBits(LED_GPIO, LED3_PIN); }

// 蜂鸣器低电平响，高电平不响
void Buzzer_On(void)  { GPIO_ResetBits(BUZZER_GPIO, BUZZER_PIN); }
void Buzzer_Off(void) { GPIO_SetBits(BUZZER_GPIO, BUZZER_PIN); }

// ==================== 实验 2-1：LED、蜂鸣器不同亮灭组合 ====================
void Exp2_1_LED_Buzzer_Combinations(void)
{
    while(1)
    {
        // 组合1：LED1~3 全亮，蜂鸣器响
        LED_All_On();
        Buzzer_On();
        delay_ms(1000);

        // 组合2：LED1~3 全亮，蜂鸣器不响
        LED_All_On();
        Buzzer_Off();
        delay_ms(1000);

        // 组合3：LED1 亮，蜂鸣器响
        LED1_On(); LED2_Off(); LED3_Off();
        Buzzer_On();
        delay_ms(1000);

        // 组合4：LED2 亮，蜂鸣器不响
        LED1_Off(); LED2_On(); LED3_Off();
        Buzzer_Off();
        delay_ms(1000);

        // 组合5：LED3 亮，蜂鸣器响
        LED1_Off(); LED2_Off(); LED3_On();
        Buzzer_On();
        delay_ms(1000);

        // 组合6：LED1、LED3 亮，LED2 灭，蜂鸣器响
        LED1_On(); LED2_Off(); LED3_On();
        Buzzer_On();
        delay_ms(1000);

        // 组合7：全灭，蜂鸣器不响
        LED_All_Off();
        Buzzer_Off();
        delay_ms(1000);
    }
}

// ==================== 实验 2-2（一）：向左流水灯 + 仅在 LED1 亮时蜂鸣器响 ====================
void Exp2_2_LeftFlow(void)
{
    while(1)
    {
        // 点亮 LED3，熄灭 LED1、LED2，蜂鸣器不响
        LED1_Off(); LED2_Off(); LED3_On();
        Buzzer_Off();
        delay_ms(500);

        // 点亮 LED2，熄灭 LED1、LED3，蜂鸣器不响
        LED1_Off(); LED2_On(); LED3_Off();
        Buzzer_Off();
        delay_ms(500);

        // 点亮 LED1，熄灭 LED2、LED3，蜂鸣器响
        LED1_On(); LED2_Off(); LED3_Off();
        Buzzer_On();    // 仅 LED1 亮时发声
        delay_ms(500);
    }
}

// ==================== 实验 2-3（原2-2二）：按键抬起检测 ====================
void Exp2_3_Button_Release(void)
{
    // 记录 LED 当前状态，0 灭，1 亮
    uint8_t led1_state = 0, led2_state = 0, led3_state = 0, buzzer_state = 0;

    while(1)
    {
        // -------- KEY1：控制 LED1 --------
        if(GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN) == 0)
        {
            delay_ms(20);
            if(GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN) == 0)
            {
                while(GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN) == 0); // 等待抬起
                delay_ms(20);
                led1_state = !led1_state;
                if(led1_state) LED1_On(); else LED1_Off();
            }
        }

        // -------- KEY2：控制 LED2 --------
        if(GPIO_ReadInputDataBit(KEY234_GPIO, KEY2_PIN) == 0)
        {
            delay_ms(20);
            if(GPIO_ReadInputDataBit(KEY234_GPIO, KEY2_PIN) == 0)
            {
                while(GPIO_ReadInputDataBit(KEY234_GPIO, KEY2_PIN) == 0);
                delay_ms(20);
                led2_state = !led2_state;
                if(led2_state) LED2_On(); else LED2_Off();
            }
        }

        // -------- KEY3：控制 LED3 --------
        if(GPIO_ReadInputDataBit(KEY234_GPIO, KEY3_PIN) == 0)
        {
            delay_ms(20);
            if(GPIO_ReadInputDataBit(KEY234_GPIO, KEY3_PIN) == 0)
            {
                while(GPIO_ReadInputDataBit(KEY234_GPIO, KEY3_PIN) == 0);
                delay_ms(20);
                led3_state = !led3_state;
                if(led3_state) LED3_On(); else LED3_Off();
            }
        }

        // -------- KEY4：控制蜂鸣器 --------
        if(GPIO_ReadInputDataBit(KEY234_GPIO, KEY4_PIN) == 0)
        {
            delay_ms(20);
            if(GPIO_ReadInputDataBit(KEY234_GPIO, KEY4_PIN) == 0)
            {
                while(GPIO_ReadInputDataBit(KEY234_GPIO, KEY4_PIN) == 0);
                delay_ms(20);
                buzzer_state = !buzzer_state;
                if(buzzer_state) Buzzer_On(); else Buzzer_Off();
            }
        }
    }
}

// ==================== 主函数：选择要运行的实验 ====================
int main(void)
{
    // 开启 GPIOA、GPIOB、GPIOC 时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    // ---------------- 配置蜂鸣器 PA8 为输出 ----------------
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_SetBits(BUZZER_GPIO, BUZZER_PIN);
    GPIO_Init(BUZZER_GPIO, &GPIO_InitStructure);

    // ---------------- 配置 LED PB5、PB0、PB1 为输出 ----------------
    GPIO_InitStructure.GPIO_Pin = LED1_PIN | LED2_PIN | LED3_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_SetBits(LED_GPIO, LED1_PIN | LED2_PIN | LED3_PIN);
    GPIO_Init(LED_GPIO, &GPIO_InitStructure);

    // ---------------- 配置 KEY1（PA0）为输入、上拉 ----------------
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(KEY1_GPIO, &GPIO_InitStructure);

    // ---------------- 配置 KEY2/3/4（PC13/PC6/PC7）为输入、上拉 ----------------
    GPIO_InitStructure.GPIO_Pin = KEY2_PIN | KEY3_PIN | KEY4_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(KEY234_GPIO, &GPIO_InitStructure);

    // ---------------- 选择运行哪个实验 ----------------
    // 请根据实验要求，只保留一个调用，其余两个注释掉

    // 实验 2-1：LED、蜂鸣器不同组合
    // Exp2_1_LED_Buzzer_Combinations();

    // 实验 2-2（一）：向左流水灯，LED1 亮时蜂鸣器响
    // Exp2_2_LeftFlow();

    // 实验 2-3：按键抬起检测
    Exp2_3_Button_Release();

    while(1);
}
