#include <stdio.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "lcd_ili9341.h"

// 简易毫秒延时（不依赖 SysTick 中断）
void Simple_Delay_Ms(uint32_t ms) {
    uint32_t i, j;
    for(i = 0; i < ms; i++) {
        for(j = 0; j < 33800; j++);  // 168MHz 下约 1ms
    }
}

int main(void) {
    UART2_Init(115200);
    Simple_Delay_Ms(100);        // 等串口稳定
    printf("LCD TEST\r\n");

    LCD_Init();

    // 手动强制开背光，确认硬件没问题
    GPIO_ResetBits(GPIOD, GPIO_Pin_12);

    LCD_SetFontEN(&ASCII_8x16);
    LCD_SetColors(RED, BLACK);
    LCD_Clear(0, 0, LCD_GetLenX(), LCD_GetLenY());
    LCD_DispStringEN(0, LINE_EN(0), 0, "3.2 inch LCD:");
    LCD_DispStringEN(0, LINE_EN(1), 0, "Image resolution:240x320 px");

    while (1) {
    }
}
