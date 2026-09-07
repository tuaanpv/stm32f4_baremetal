#ifndef __I2C_H
#define __I2C_H

#include "stm32f4xx.h"

/* I2C1: SCL = PB6, SDA = PB9 (AF4), Standard mode 100kHz, APB1 = 16MHz (HSI, PLL not configured) */
void i2c1_init(void);

/* Write n bytes to device at 7-bit address dev_addr, starting at reg */
int i2c1_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);

/* Read n bytes from device at 7-bit address dev_addr, starting at reg */
int i2c1_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

/* ---- Interrupt-driven read (non-blocking) ---- */

typedef enum {
    I2C_IT_IDLE = 0,
    I2C_IT_BUSY,
    I2C_IT_DONE,
    I2C_IT_ERROR,
} i2c_it_status_t;

/* Same pin/clock/CCR setup as i2c1_init(), plus enables I2C1's event and
   error interrupts and their NVIC lines. Once called, do not use the
   blocking i2c1_read()/i2c1_write() on I2C1 anymore -- the event interrupt
   fires on the same SB/ADDR/BTF/RXNE flags those functions poll, so the two
   would race over the same hardware state. */
void i2c1_it_init(void);

/* Starts an asynchronous read of len bytes starting at register reg on
   device dev_addr. Returns immediately (I2C_IT_BUSY) without waiting for
   the transfer to finish; poll i2c1_it_get_status() to know when data (in
   the caller-owned buf) is ready. Returns I2C_IT_ERROR immediately, without
   starting anything, if a previous transaction is still in flight. */
i2c_it_status_t i2c1_it_read(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len);

/* Current state of the most recent i2c1_it_read() call. */
i2c_it_status_t i2c1_it_get_status(void);

/* The actual ISRs (called by the CPU via the vector table, not by
   application code). Declared here so i2c.c's definitions are visible
   project-wide, but normal code never calls them directly. */
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);

#endif
