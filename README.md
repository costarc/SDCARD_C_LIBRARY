# SDCARD_C_LIBRARY
This library brings together FatFS and low-level I/O functions to access SD card files and directories using either hardware SPI or bit-banged signals.

## Folders and Implementations
### device_sd_spi
Implements SD card access using the Raspberry Pi's native SPI on GPIOs 8, 9, 10, and 11.
Requires SPI to be enabled. To enable SPI, run:

sudo raspi-config
Then go to:
Interface Options → SPI → Enable, and reboot the Raspberry Pi if prompted.

### device_sd_bitbang
Implements SD card access by bit-banging GPIOs 8, 9, 10, and 11.
Requires hardware SPI to be disabled. To disable SPI, run:

sudo raspi-config
Then go to:
Interface Options → SPI → Disable, and reboot the Raspberry Pi.

This implementation is intended to be portable to other devices that do not have native SPI hardware support.

### sdcc_sd_bitbang
An untested version of SD card access via bit-banging on a Z80 system.
The code compiles, but has not been tested. Some low-level C functions (fgets, scanf, etc.) were reimplemented for compatibility, but they have not been verified and likely contain bugs.

⚠️ This version is experimental. Use at your own risk.

### device_file
Imcomplete experimental versoin, that creates a device in a local file.
⚠️ This version is experimental. Use at your own risk.
