gcc -o filebrowser filebrowser.c diskio.c ../FatFS/source/ff.c ../FatFS/source/ffsystem.c ../FatFS/source/ffunicode.c FILE_disk_initialize.c FILE_disk_read.c FILE_disk_write.c FILE_ioctl.c 
