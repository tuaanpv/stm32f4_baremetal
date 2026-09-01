#include "sdcard.h"
#include "spi.h"

#define SD_CMD_TIMEOUT     8      /* max bytes to poll while waiting for R1 */
#define SD_ACMD41_TIMEOUT  20000  /* max loop iterations while waiting for the card to leave idle state */
#define SD_TOKEN_TIMEOUT   100000 /* max loop iterations while waiting for a data token */

static int s_is_sdhc = 0;

static void sd_dummy_clocks(int bytes)
{
    /* Send 0xFF bytes with CS released; the card needs extra clock
       pulses after most operations to finish internal processing. */
    for (int i = 0; i < bytes; i++) {
        spi1_transfer_byte(0xFF);
    }
}

/* Sends a command frame and returns the R1 response byte.
   crc only matters for CMD0 and CMD8, since CRC checking is disabled
   by default in SPI mode for every other command. */
static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    spi1_transfer_byte(0xFF); /* one idle byte before the command, per SD spec */
    spi1_transfer_byte(0x40 | cmd);
    spi1_transfer_byte((uint8_t)(arg >> 24));
    spi1_transfer_byte((uint8_t)(arg >> 16));
    spi1_transfer_byte((uint8_t)(arg >> 8));
    spi1_transfer_byte((uint8_t)arg);
    spi1_transfer_byte(crc);

    uint8_t r1 = 0xFF;
    for (int i = 0; i < SD_CMD_TIMEOUT; i++) {
        r1 = spi1_transfer_byte(0xFF);
        if ((r1 & 0x80) == 0) break; /* bit7=0 means a valid R1 byte arrived */
    }
    return r1;
}

int sd_is_high_capacity(void)
{
    return s_is_sdhc;
}

sd_status_t sd_init(void)
{
    s_is_sdhc = 0;

    spi1_set_baudrate_slow(); /* card init must happen below 400kHz */
    spi1_cs_high();
    sd_dummy_clocks(10); /* >=74 clock cycles with CS high and MOSI high, before any command */

    spi1_cs_low();
    uint8_t r1 = sd_cmd(0, 0, 0x95); /* CMD0: GO_IDLE_STATE, enters SPI mode */
    spi1_cs_high();
    sd_dummy_clocks(1);
    if (r1 != 0x01) {
        return SD_ERR_CMD0; /* card did not respond / is not present */
    }

    spi1_cs_low();
    r1 = sd_cmd(8, 0x1AA, 0x87); /* CMD8: SEND_IF_COND, checks voltage range + card version */
    uint8_t r7[4] = {0};
    int is_v2 = (r1 == 0x01);
    if (is_v2) {
        for (int i = 0; i < 4; i++) r7[i] = spi1_transfer_byte(0xFF);
    }
    spi1_cs_high();
    sd_dummy_clocks(1);
    if (is_v2 && !(r7[2] == 0x01 && r7[3] == 0xAA)) {
        return SD_ERR_CMD8; /* card echoed back something other than what we sent */
    }
    /* Note: cards that reject CMD8 (illegal command) are old MMC/SDv1 cards;
       this driver targets SDv2 (SDHC/SDXC and standard SDv2), which covers
       essentially all cards sold in the last ~15 years. */

    /* ACMD41 (via CMD55 + CMD41) until the card leaves idle state.
       Set HCS (bit 30) to tell the card we support high-capacity addressing. */
    int acmd41_ok = 0;
    for (int i = 0; i < SD_ACMD41_TIMEOUT; i++) {
        spi1_cs_low();
        sd_cmd(55, 0, 0x01); /* CMD55: APP_CMD, next command is an ACMD */
        r1 = sd_cmd(41, is_v2 ? 0x40000000UL : 0, 0x01); /* ACMD41: SD_SEND_OP_COND */
        spi1_cs_high();
        sd_dummy_clocks(1);
        if (r1 == 0x00) { acmd41_ok = 1; break; }
    }
    if (!acmd41_ok) {
        return SD_ERR_ACMD41_TIMEOUT;
    }

    /* CMD58: READ_OCR, check the CCS bit to know if the card uses block
       addressing (SDHC/SDXC) or byte addressing (standard capacity). */
    spi1_cs_low();
    r1 = sd_cmd(58, 0, 0x01);
    uint8_t ocr[4] = {0};
    if (r1 == 0x00) {
        for (int i = 0; i < 4; i++) ocr[i] = spi1_transfer_byte(0xFF);
    }
    spi1_cs_high();
    sd_dummy_clocks(1);
    if (r1 != 0x00) {
        return SD_ERR_CMD58;
    }
    s_is_sdhc = (ocr[0] & 0x40) ? 1 : 0; /* CCS = OCR bit 30 */

    spi1_set_baudrate_fast(); /* init sequence done, switch to normal transfer speed */
    return SD_OK;
}

sd_status_t sd_read_block(uint32_t block_addr, uint8_t *buf)
{
    /* SDHC/SDXC cards address blocks directly; standard-capacity cards
       expect a byte offset instead. */
    uint32_t addr = s_is_sdhc ? block_addr : (block_addr * SD_BLOCK_SIZE);

    spi1_cs_low();
    uint8_t r1 = sd_cmd(17, addr, 0x01); /* CMD17: READ_SINGLE_BLOCK */
    if (r1 != 0x00) {
        spi1_cs_high();
        return SD_ERR_NO_RESPONSE;
    }

    uint8_t token = 0xFF;
    int got_token = 0;
    for (int i = 0; i < SD_TOKEN_TIMEOUT; i++) {
        token = spi1_transfer_byte(0xFF);
        if (token == 0xFE) { got_token = 1; break; } /* 0xFE = start-of-data token */
    }
    if (!got_token) {
        spi1_cs_high();
        return SD_ERR_READ_TOKEN;
    }

    for (int i = 0; i < SD_BLOCK_SIZE; i++) {
        buf[i] = spi1_transfer_byte(0xFF);
    }
    spi1_transfer_byte(0xFF); /* CRC16, ignored (CRC checking is off by default) */
    spi1_transfer_byte(0xFF);

    spi1_cs_high();
    sd_dummy_clocks(1);
    return SD_OK;
}

sd_status_t sd_write_block(uint32_t block_addr, const uint8_t *buf)
{
    uint32_t addr = s_is_sdhc ? block_addr : (block_addr * SD_BLOCK_SIZE);

    spi1_cs_low();
    uint8_t r1 = sd_cmd(24, addr, 0x01); /* CMD24: WRITE_BLOCK */
    if (r1 != 0x00) {
        spi1_cs_high();
        return SD_ERR_NO_RESPONSE;
    }

    spi1_transfer_byte(0xFE); /* start-of-data token */
    for (int i = 0; i < SD_BLOCK_SIZE; i++) {
        spi1_transfer_byte(buf[i]);
    }
    spi1_transfer_byte(0xFF); /* dummy CRC16, ignored by the card since CRC is off */
    spi1_transfer_byte(0xFF);

    uint8_t resp = spi1_transfer_byte(0xFF);
    if ((resp & 0x1F) != 0x05) { /* data response token: xxx00101 = accepted */
        spi1_cs_high();
        return SD_ERR_WRITE_RESPONSE;
    }

    /* Card pulls MISO low while busy programming flash; wait for it to release. */
    int busy_timeout = SD_TOKEN_TIMEOUT;
    while (spi1_transfer_byte(0xFF) == 0x00 && --busy_timeout > 0) {
    }

    spi1_cs_high();
    sd_dummy_clocks(1);
    return (busy_timeout > 0) ? SD_OK : SD_ERR_WRITE_RESPONSE;
}
