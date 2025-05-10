SDCARD_C_LIBRARY
This library integrates FatFS with low-level I/O functions to access SD card files and directories using either hardware SPI or bit-banged SPI.

The following folders contain tested and working code:

device_sd_spi: Implements SD card access using the Raspberry Pi's native SPI on GPIOs 8, 9, 10, and 11. Requires SPI to be enabled (sudo raspi-config → enable SPI).

device_sd_bitbang: Implements SD card access by bit-banging GPIOs 8, 9, 10, and 11. Requires SPI to be disabled (sudo raspi-config → disable SPI, then reboot). This implementation is intended to be portable to devices that do not have hardware SPI support.

sdcc_sd_bitbang: An untested version of SD card access via bit-banging on a Z80 system. The code compiles, but has not been tested. Some low-level C functions like fgets and scanf were rewritten, but they may contain bugs.
