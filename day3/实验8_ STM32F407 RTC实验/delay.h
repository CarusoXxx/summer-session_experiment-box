#ifndef __DELAY_H__
#define __DELAY_H__
#include "stm32f4xx.h"

void systick_delay_us(uint32_t us);

#define delay_us(x)  systick_delay_us(x)
#define delay_ms(x)  systick_delay_us((x) * 1000)

#endif