#include <stdio.h>
#include "../FatFS/source/ff.h"
#include "../FatFS/source/diskio.h"
#include "diskio_lowlevel.h"

typedef unsigned int DWORD;

DSTATUS FILE_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    DSTATUS res = RES_ERROR;

    switch (cmd) {
        case CTRL_SYNC:
            printf("Synchronizing disk %u...\n", pdrv);
            res = RES_OK; // Perform synchronization if needed
            break;

        case GET_SECTOR_COUNT:
            printf("Getting sector count for disk %u...\n", pdrv);
            // Assuming a 1GB disk with 512 byte sectors (example)
            *((DWORD*)buff) = (FILE_DISK_SIZE * 1024) / FILE_SECTOR_SIZE;  // Set the number of sectors to 2048 * 1024
            res = RES_OK;
            break;

        case GET_BLOCK_SIZE:
            printf("Getting block size for disk %u...\n", pdrv);
            *((DWORD*)buff) = 512;  // Example: 512 bytes per block
            res = RES_OK;
            break;

        case GET_SECTOR_SIZE:
            printf("Getting sector size for disk %u...\n", pdrv);
            *((DWORD*)buff) = 512;  // Example: 512 bytes per sector
            res = RES_OK;
            break;

        case CTRL_TRIM:
            printf("Trimming off disk %u...\n", pdrv);
            res = RES_OK;
            break;

        default:
            printf("Invalid ioctl command for disk %u.\n", pdrv);
            break;
    }

    return res;
}
