#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "../FatFS/source/ff.h"
#include "../FatFS/source/diskio.h"
#include "diskio_lowlevel.h"

DSTATUS FILE_disk_initialize(BYTE pdrv) {
    printf("Initializing disk to use...\n");
    FILE *file;
    size_t size_in_mb = FILE_DISK_SIZE * 1024;  // 1MB size in bytes

    // Construct the filename string: disk<pdrv>.dsk
    char filename[13];  // Filename using 8.3 format + line end
    sprintf(filename, "disk%u.dsk", pdrv);

    // Try to open the file in read mode to check if it exists
    file = fopen(filename, "rb");
    
    if (file != NULL) {
        // If the file exists, close it and return RES_OK
        fclose(file);
        printf("Disk %u Already Exist.\n", pdrv);
        return RES_OK;
    } else {
        // If the file does not exist, create a new file of 1MB size
        file = fopen(filename, "wb");
        
        if (file == NULL) {
            // If file creation failed, return RES_ERROR
            return RES_ERROR;
        }
        
        // Fill the file with 1MB of zeros (for simplicity, or you can choose some other value)
        char zero = 0;
        for (size_t i = 0; i < size_in_mb; i++) {
            fwrite(&zero, 1, 1, file);
        }
        
        // Close the file after writing
        fclose(file);
        
        // Return RES_OK after successfully creating the file
        return RES_OK;
    }
}
