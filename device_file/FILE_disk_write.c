#include <stdio.h>
#include <stdlib.h>
#include "../FatFS/source/ff.h"
#include "../FatFS/source/diskio.h"
#include "diskio_lowlevel.h"

DRESULT FILE_disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    char filename[13];
    FILE *file;
    size_t offset;
    size_t write_size;

    // Construct the file name: disk<pdrv>.dsk
    snprintf(filename, sizeof(filename), "disk%u.dsk", pdrv);

    // Open the file in read+binary update mode
    file = fopen(filename, "rb+");
    if (file == NULL) {
        return RES_ERROR;
    }

    // Calculate the offset to start writing: sector * FILE_SECTOR_SIZE
    offset = sector * FILE_SECTOR_SIZE;

    // Move the file pointer to the correct sector position
    if (fseek(file, offset, SEEK_SET) != 0) {
        fclose(file);
        return RES_ERROR;
    }

    // Calculate total bytes to write
    write_size = count * FILE_SECTOR_SIZE;

    // Write data from buffer into file
    size_t written = fwrite(buff, 1, write_size, file);
    fclose(file);

    if (written != write_size) {
        return RES_ERROR;
    }

    return RES_OK;
}
