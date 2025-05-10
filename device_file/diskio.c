/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "time.h"
#include "stdio.h"
#include "../FatFS/source/ff.h"			/* Obtains integer types */
#include "../FatFS/source/diskio.h"		/* Declarations of disk functions */
#include "diskio_lowlevel.h"

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */
#define DEV_FILE	3	/* Example: Map FILE to physical drive 3 */


/*-----------------------------------------------------------------------*/
/* Custom low-level routines called by  Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DWORD get_fattime(void) {
    time_t t = time(NULL);
    struct tm now;
    
    // Convert time to local time (thread-safe)
    if (localtime_r(&t, &now) == NULL) {
        return 0;  // fallback if time is not available
    }

    return ((DWORD)(now.tm_year - 80) << 25)
         | ((DWORD)(now.tm_mon + 1) << 21)
         | ((DWORD)now.tm_mday << 16)
         | ((DWORD)now.tm_hour << 11)
         | ((DWORD)now.tm_min << 5)
         | ((DWORD)(now.tm_sec / 2));
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nuMber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
	case DEV_RAM :
		return RES_NOTRDY;

	case DEV_MMC :
		return RES_NOTRDY;

	case DEV_USB :
		return RES_NOTRDY;
		
	case DEV_FILE :
	    char filename[13];  // Filename using 8.3 format + line end
    	sprintf(filename, "disk%u.dsk", pdrv);
		FILE *file = fopen(filename, "r");
    	if (file) {
        	fclose(file);
        	stat = RES_OK;  // File exists
    	} else {
        	stat = RES_NOTRDY;
        }

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
	case DEV_RAM :
		return RES_NOTRDY;

	case DEV_MMC :
		return RES_NOTRDY;

	case DEV_USB :
		return RES_NOTRDY;
		
	case DEV_FILE :
		stat = FILE_disk_initialize(pdrv);
		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;

	switch (pdrv) {
	
	case DEV_RAM :
		return RES_ERROR;

	case DEV_MMC :
		return RES_ERROR;

	case DEV_USB :
		return RES_ERROR;
		
	case DEV_FILE :
		// translate the arguments here

		//result = FILE_disk_read(buff, sector, count);

		// translate the reslut code here

		res = RES_ERROR;
		
		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;

	switch (pdrv) {
	case DEV_RAM :
		return RES_ERROR;

	case DEV_MMC :
		return RES_ERROR;

	case DEV_USB :
		return RES_ERROR;
		
	case DEV_FILE :
		// translate the arguments here

		// result = FILE_disk_write(buff, sector, count);

		// translate the reslut code here

		res = RES_ERROR;
		return res;
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;

	switch (pdrv) {
	case DEV_RAM :

		// Process of the command for the RAM drive

		return RES_ERROR;

	case DEV_MMC :

		// Process of the command for the MMC/SD card

		return RES_ERROR;

	case DEV_USB :

		// Process of the command the USB drive

		return RES_ERROR;
		
	case DEV_FILE :

		// Process of the command the FILE drive

		res = FILE_ioctl(pdrv, cmd, buff);
		return res;
	}


	return RES_PARERR;
}

