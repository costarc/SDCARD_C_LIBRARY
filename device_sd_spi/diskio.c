#include "../FatFS/source/ff.h"
#include "../FatFS/source/diskio.h"
#include <stdint.h>
#include <stdio.h>

extern int sd_init();
extern int sd_read_sectors(uint32_t lba, uint8_t *buffer, int count);

#define DEV_SD	0

DSTATUS disk_initialize(BYTE pdrv) {
	if (pdrv != DEV_SD) return STA_NOINIT;
	return (sd_init() == 0) ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv) {
	if (pdrv != DEV_SD) return STA_NOINIT;
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
	if (pdrv != DEV_SD) return RES_PARERR;
	return (sd_read_sectors(sector, buff, count) == 0) ? RES_OK : RES_ERROR;
}

extern int sd_write_sectors(uint32_t lba, const uint8_t *buffer, int count);

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
	if (pdrv != DEV_SD) return RES_PARERR;
	return (sd_write_sectors(sector, buff, count) == 0) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
	if (pdrv != DEV_SD) return RES_PARERR;

	switch (cmd) {
		case CTRL_SYNC:
			return RES_OK;
		case GET_SECTOR_COUNT:
			*(DWORD*)buff = 32768; // Dummy: 16MB card (512 * 32768)
			return RES_OK;
		case GET_SECTOR_SIZE:
			*(WORD*)buff = 512;
			return RES_OK;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = 1;
			return RES_OK;
	}

	return RES_PARERR;
}

// Optional: implement if you want to support timestamps
DWORD get_fattime(void) {
	return ((DWORD)(2025 - 1980) << 25) // Year
		 | ((DWORD)5 << 21)				// Month
		 | ((DWORD)3 << 16)				// Day
		 | ((DWORD)12 << 11)			// Hour
		 | ((DWORD)0 << 5)				// Minute
		 | ((DWORD)0 >> 1);				// Second
}

