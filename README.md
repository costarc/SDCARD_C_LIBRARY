# SDCARD_C_LIBRARY

This library integrates **FatFS** with low-level I/O functions to access SD card files and directories using either **hardware SPI** or **bit-banged SPI**.

## Available Implementations

### `device_sd_spi`
Implements SD card access using the Raspberry Pi's native SPI on GPIOs **8, 9, 10, and 11**.  
Requires SPI to be enabled:

```bash
sudo raspi-config
# Navigate to Interface Options → SPI → Enable
