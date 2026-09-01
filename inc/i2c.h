#ifndef __I2C_H
#define __I2C_H

#include "stm32f4xx.h"

/* I2C1: SCL = PB6, SDA = PB9 (AF4), Standard mode 100kHz, APB1 = 16MHz (HSI, PLL not configured) */
void i2c1_init(void);

/* Write n bytes to device at 7-bit address dev_addr, starting at reg */
int i2c1_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);

/* Read n bytes from device at 7-bit address dev_addr, starting at reg */
int i2c1_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

#endif
