#ifndef __SPI_H
#define __SPI_H

#include "stm32f4xx.h"

/* SPI1: SCK = PB3, MISO = PB4, MOSI = PB5 (AF5). CS is a plain GPIO
   (PA4), driven manually so any SPI chip select scheme can be layered
   on top (SD card, flash chip, etc.). NSS hardware management is not
   used (SSM/SSI set instead). */

void spi1_init(void);
void spi1_set_baudrate_slow(void); /* ~clk/256, required during SD card init (<400kHz) */
void spi1_set_baudrate_fast(void); /* ~clk/4, used for normal transfers after init */
uint8_t spi1_transfer_byte(uint8_t out);

void spi1_cs_low(void);
void spi1_cs_high(void);

#endif
