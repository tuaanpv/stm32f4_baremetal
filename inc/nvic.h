#ifndef __NVIC_H
#define __NVIC_H

#include "stm32f4xx.h"

/* irqn: the IRQ number from the vector table (IRQ0-based, e.g. USART2_IRQn) */
void nvic_enable_irq(uint8_t irqn);
void nvic_disable_irq(uint8_t irqn);

#endif
