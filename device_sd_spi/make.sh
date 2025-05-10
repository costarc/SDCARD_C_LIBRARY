gcc -o sdcard_spi sdcard_spi.c diskio.c ../FatFS/source/ff.c -lpigpio

echo "This code requires SPI enabled in the Raspberry pi. Use sudo raspi-config to enable, then reboot"
echo "Run with sudo ./sdcard_spi"
