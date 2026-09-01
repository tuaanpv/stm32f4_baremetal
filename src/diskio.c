/**
 * diskio.c
 * FatFs low-level disk I/O glue for this project's SPI-mode SD card driver
 * (sdcard.c/sdcard.h). This is the file every FatFs port is expected to
 * provide itself -- FatFs only ships an empty diskio.template.c.
 *
 * Requires the official diskio.h and ff.h from the FatFs distribution
 * to be present in inc/ (see elm-chan.org/fsw/ff/00index_e.html).
 */

#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include "spi.h"

/* This project only ever talks to one physical drive, so pdrv is expected
   to always be 0; the argument is still checked for a safety margin. */
#define SD_DRIVE_NUM 0

static volatile DSTATUS s_disk_status = STA_NOINIT;

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != SD_DRIVE_NUM) return STA_NOINIT;
    return s_disk_status;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != SD_DRIVE_NUM) return STA_NOINIT;

    spi1_init();
    sd_status_t st = sd_init();
    s_disk_status = (st == SD_OK) ? 0 : STA_NOINIT;
    return s_disk_status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != SD_DRIVE_NUM) return RES_PARERR;
    if (s_disk_status & STA_NOINIT) return RES_NOTRDY;

    for (UINT i = 0; i < count; i++) {
        if (sd_read_block((uint32_t)sector + i, buff + (i * SD_BLOCK_SIZE)) != SD_OK) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != SD_DRIVE_NUM) return RES_PARERR;
    if (s_disk_status & STA_NOINIT) return RES_NOTRDY;

    for (UINT i = 0; i < count; i++) {
        if (sd_write_block((uint32_t)sector + i, buff + (i * SD_BLOCK_SIZE)) != SD_OK) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != SD_DRIVE_NUM) return RES_PARERR;
    if (s_disk_status & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            /* All writes in sd_write_block() already wait for the card to
               leave the busy state before returning, so there is nothing
               left to flush here. */
            return RES_OK;

        case GET_SECTOR_COUNT: {
            /* This driver does not read CSD to compute the real capacity;
               report a conservative placeholder so f_mkfs has a plausible
               volume size. Replace with a real CSD-based calculation if
               exact capacity reporting is needed. */
            *(LBA_t *)buff = 1UL * 1024 * 1024; /* ~512MB assumed at 512B/sector */
            return RES_OK;
        }

        case GET_SECTOR_SIZE:
            *(WORD *)buff = SD_BLOCK_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1; /* erase block size unknown; 1 = no info */
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

/* FatFs needs a timestamp source for file creation/modification dates.
   This project has no RTC configured, so it returns a fixed placeholder
   date instead of a real one. Wire this up to an RTC peripheral if
   accurate file timestamps are needed. */
DWORD get_fattime(void)
{
    /* Bit layout required by FatFs: year(from 1980)<<25 | month<<21 | day<<16
       | hour<<11 | minute<<5 | second/2. Fixed at 2026-01-01 00:00:00. */
    return ((DWORD)(2026 - 1980) << 25) | ((DWORD)1 << 21) | ((DWORD)1 << 16);
}
