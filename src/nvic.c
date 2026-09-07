#include "nvic.h"

void nvic_enable_irq(uint8_t irqn)
{
    NVIC->ISER[irqn >> 5] = (1UL << (irqn & 0x1F));
}

void nvic_disable_irq(uint8_t irqn)
{
    NVIC->ICER[irqn >> 5] = (1UL << (irqn & 0x1F));
}
