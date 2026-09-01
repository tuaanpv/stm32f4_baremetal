#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"

/* sysclk_hz: actual system clock frequency fed to SysTick (Hz) */
void delay_init(uint32_t sysclk_hz);
void delay_ms(uint32_t ms);

#endif
