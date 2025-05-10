gcc -o sdcard_bitbang sdcard_bitbang.c diskio.c ../FatFS/source/ff.c -lpigpio
echo "This code requires SPI to be DISABLED, because it is using the SPI GPIOs. Disable SPI with sudo raspi-config, then reboot"
echo "Run with sudo ./sdcard_bitbang"

