#define FILE_SECTOR_SIZE 512
#define FILE_DISK_SIZE 1024

DSTATUS FILE_ioctl(BYTE pdrv, BYTE cmd, void *buff);
DSTATUS FILE_disk_initialize(BYTE pdrv);
DRESULT FILE_disk_read (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
DRESULT FILKE_disk_write (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
