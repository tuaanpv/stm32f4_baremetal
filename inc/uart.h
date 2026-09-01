#ifndef __UART_H
#define __UART_H

#include "stm32f4xx.h"

/* USART2: TX = PA2, RX = PA3 (AF7). Polling mode, 8N1. */
void uart2_init(uint32_t pclk_hz, uint32_t baud);
void uart2_write_byte(uint8_t byte);
void uart2_write_string(const char *str);
void uart2_write_uint(uint32_t value); /* decimal, no leading zeros */
void uart2_write_hex_byte(uint8_t value); /* 2 hex digits, e.g. "3F" */
uint8_t uart2_read_byte(void);         /* blocking read */

#endif
