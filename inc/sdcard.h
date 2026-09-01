#ifndef __SDCARD_H
#define __SDCARD_H

#include "stm32f4xx.h"

#define SD_BLOCK_SIZE 512

typedef enum {
    SD_OK = 0,
    SD_ERR_NO_RESPONSE = -1,
    SD_ERR_CMD0 = -2,
    SD_ERR_CMD8 = -3,
    SD_ERR_ACMD41_TIMEOUT = -4,
    SD_ERR_CMD58 = -5,
    SD_ERR_READ_TOKEN = -6,
    SD_ERR_WRITE_RESPONSE = -7,
} sd_status_t;

/* Runs the SD card SPI-mode power-up/init sequence (CMD0, CMD8, ACMD41, CMD58).
   Must be called with spi1_init() already done. Leaves the SPI bus at the
   "fast" baud rate for normal read/write use afterward. */
sd_status_t sd_init(void);

/* True if the card identified itself as SDHC/SDXC (block-addressed) during
   sd_init(). Relevant only for diagnostics — sd_read_block/sd_write_block
   already account for it internally. */
int sd_is_high_capacity(void);

/* Reads one 512-byte block. block_addr is the block number (0-based), NOT a byte offset. */
sd_status_t sd_read_block(uint32_t block_addr, uint8_t *buf);

/* Writes one 512-byte block. block_addr is the block number (0-based). */
sd_status_t sd_write_block(uint32_t block_addr, const uint8_t *buf);

#endif
