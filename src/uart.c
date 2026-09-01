#include "uart.h"

void uart2_init(uint32_t pclk_hz, uint32_t baud)
{
    /* Enable GPIOA and USART2 clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = TX, PA3 = RX, Alternate Function, AF7 = USART2 */
    for (int i = 0; i < 2; i++) {
        uint8_t pin = (i == 0) ? 2 : 3;
        GPIOA->MODER &= ~(0x3UL << (pin * 2));
        GPIOA->MODER |=  (GPIO_MODE_AF << (pin * 2));
        GPIOA->OSPEEDR |= (0x3UL << (pin * 2)); /* high speed */

        uint8_t af_reg = pin / 8;
        uint8_t af_shift = (pin % 8) * 4;
        GPIOA->AFR[af_reg] &= ~(0xFUL << af_shift);
        GPIOA->AFR[af_reg] |=  (0x7UL << af_shift); /* AF7 = USART2 */
    }

    /* Baud rate: USARTDIV = pclk / (16 * baud), computed *100 to keep the
       fractional part without floating point, then split into
       mantissa (bits 15:4) and 4-bit fraction (bits 3:0). */
    uint32_t usartdiv_x100 = (25UL * pclk_hz) / (4UL * baud);
    uint32_t mantissa = usartdiv_x100 / 100UL;
    uint32_t fraction = (((usartdiv_x100 - mantissa * 100UL) * 16UL) + 50UL) / 100UL;
    if (fraction > 15UL) { /* carry into mantissa if rounding overflowed the fraction field */
        fraction -= 16UL;
        mantissa += 1UL;
    }
    USART2->BRR = (mantissa << 4) | (fraction & 0xFUL);

    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; /* 8N1, no parity by default */
}

void uart2_write_byte(uint8_t byte)
{
    while ((USART2->SR & USART_SR_TXE) == 0) { /* wait until the data register is empty */
    }
    USART2->DR = byte;
}

void uart2_write_string(const char *str)
{
    while (*str) {
        uart2_write_byte((uint8_t)*str++);
    }
}

void uart2_write_uint(uint32_t value)
{
    char buf[10]; /* max 10 digits for a 32-bit value */
    int i = 0;

    if (value == 0) {
        uart2_write_byte('0');
        return;
    }
    while (value > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (i > 0) {
        uart2_write_byte((uint8_t)buf[--i]);
    }
}

void uart2_write_hex_byte(uint8_t value)
{
    static const char hex_digits[] = "0123456789ABCDEF";
    uart2_write_byte((uint8_t)hex_digits[(value >> 4) & 0xF]);
    uart2_write_byte((uint8_t)hex_digits[value & 0xF]);
}

uint8_t uart2_read_byte(void)
{
    while ((USART2->SR & USART_SR_RXNE) == 0) { /* wait until a byte has arrived */
    }
    return (uint8_t)USART2->DR;
}
