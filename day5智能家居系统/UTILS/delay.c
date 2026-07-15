#include "delay.h"
#include "stm32f4xx.h"

// ===== 基于 SysTick 的精准微秒延时（推荐 DHT11 使用这个） =====
void systick_delay_us(uint32_t us) {
    uint32_t ticks = 0;
    uint32_t told, tnow, reload, tcnt = 0;

    reload = SysTick->LOAD;
    ticks = us * (SystemCoreClock / 1000000);
    told = SysTick->VAL;

    while (1) {
        tnow = SysTick->VAL;
        if (tnow == told) {
            continue;
        }
        if (tnow < told) {
            tcnt += told - tnow;
        } else {
            tcnt += reload - tnow + told;
        }
        told = tnow;
        if (tcnt >= ticks) {
            break;
        }
    }
}

// ===== 空循环延时（粗略，用于毫秒级延时） =====
void delay_us(uint16_t nus) {
    uint16_t i;
    while (nus--) {
        i = 31;          // 168MHz 下约 1μs
        while (i--);
    }
}

void delay_ms(uint16_t nms) {
    uint16_t i;
    while (nms--) {
        i = 33800;       // 168MHz 下约 1ms
        while (i--);
    }
}
