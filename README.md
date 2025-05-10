# SDCARD_C_LIBRARY
Puts together FatFS and the low-level I/O functions to access SDCARD files and dirctories using hardware SPI or bitbang the signals.

These folders contains tested and working code:

device_sd_spi: Implements sdcard access using Rapsberry Pi native SPI on GPIOs 8,9,10,11. Requires SPI to be enabled (use sudo rapsi-config to enable SPI).

device_sd_bitbang: Implements sdcard access bitbanging the GPIOs 8,9,10,11. Requires SPI disabled (use sudo raspi-config to disabled the native SPI, then reboot Raspberry Pi). This implementation has the objective to be portable to other devices that do not have SPI implementation.

sdcc_sd_bitbang: Not tested version of the sdcard bitbang access on a Z80 system. This version is compiled, but not tested. Some low level C functions had to be re-writen (fgets, scanf) but they were not tested, there are probalby, many bugs.