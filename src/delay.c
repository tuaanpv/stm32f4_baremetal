#include "delay.h"

static uint32_t ticks_per_ms;

void delay_init(uint32_t sysclk_hz)
{
    ticks_per_ms = sysclk_hz / 1000UL;
    /* SysTick uses the core clock directly (CLKSOURCE=1), no interrupt,
       just polls COUNTFLAG for simplicity. */
    SysTick->CTRL = 0;
}

void delay_ms(uint32_t ms)
{
    SysTick->LOAD = (ticks_per_ms - 1) & 0x00FFFFFF;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_ENABLE;

    for (uint32_t i = 0; i < ms; i++) {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG) == 0) {
            /* wait for 1ms to elapse */
        }
    }
    SysTick->CTRL = 0;
}
