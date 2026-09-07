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

/* ---- Interrupt-driven RX (non-blocking) ---- */

/* Same pin/baud setup as uart2_init(), plus enables the RXNE interrupt and
   NVIC so incoming bytes land in a ring buffer instead of being read by
   blocking uart2_read_byte(). Do not mix uart2_read_byte() with this once
   called -- the ISR already consumes RXNE, so the blocking read would race
   with it and could hang waiting for a flag the ISR already cleared. */
void uart2_it_init(uint32_t pclk_hz, uint32_t baud);

/* Pops one byte from the RX ring buffer. Returns 1 and writes *out if a
   byte was available, 0 if the buffer is currently empty (non-blocking). */
int uart2_it_read_byte(uint8_t *out);

/* The actual ISR (called by the CPU via the vector table, not by application
   code). Declared here so uart.c's definition is visible project-wide, but
   normal code never calls it directly. */
void USART2_IRQHandler(void);

#endif
