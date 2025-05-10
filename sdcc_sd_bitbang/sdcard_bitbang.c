#define MOSI_PIN 10	 // GPIO10 (SPI0_MOSI) // Yellow
#define MISO_PIN 9	 // GPIO9  (SPI0_MISO) // Orange
#define SCLK_PIN 11	 // GPIO11 (SPI0_SCLK) // Green
#define CS_PIN 8	 // GPIO8  (SPI0_CE0)  // Blue
// 3V  - Red
// GND - Brown

// CP400 Breakout (From bottom, pins 39 - 40):
// RED BROWD
// - -
// - -
// - -
// OR BL
// GR YE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "../FatFS/source/ff.h" // FatFs library header
#include "../FatFS/source/diskio.h" // Disk I/O library header

// SD command constants
#define CMD0  0
#define CMD1  1
#define CMD8  8
#define CMD17 17
#define CMD24 24  // Write single block command
#define CMD25 25  // Write multiple blocks command
#define CMD41 41
#define CMD58 58
#define CMD55 55

// Z80 Defines
#ifndef SDCARD_Z80_H
#define SDCARD_Z80_H

#define PORT_MOSI  0x10
#define PORT_MISO  0x11
#define PORT_SCLK  0x12
#define PORT_CS	   0x13

__sfr __at(PORT_MOSI) spi_mosi;
__sfr __at(PORT_MISO) spi_miso;
__sfr __at(PORT_SCLK) spi_sclk;
__sfr __at(PORT_CS)	  spi_cs;

// Macros to simulate gpioWrite / gpioRead
#define SET_CS(val)		 (spi_cs = (val))
#define SET_MOSI(val)	 (spi_mosi = (val))
#define SET_SCLK(val)	 (spi_sclk = (val))
#define READ_MISO()		 (spi_miso & 1)

#endif

static int card_is_sdhc = 0;
static int spi_handle = -1;
FATFS fs;
char sd_path[256] = "0:";  // Initial SD root directory

void delay_ms(uint16_t ms) {
	// Simple loop-based delay (adjust for your clock speed)
	for (uint16_t i = 0; i < ms; i++) {
		for (uint16_t j = 0; j < 400; j++) __asm__("nop");
	}
}

void scanf(char *buffer, unsigned int maxlen) {
	unsigned int i = 0;
	char c;

	// Skip leading whitespace
	do {
		c = getchar();
	} while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

	// Read until newline, space, or EOF
	while (i < maxlen - 1 && (int)c != EOF && c != '\n') {
		buffer[i++] = c;
		c = getchar();
	}

	buffer[i] = '\0';  // Null-terminate the string
}

char* fgets(char* buffer, int maxlen) {
	int i = 0;
	int c;

	// Read characters until newline, EOF, or buffer full
	while (i < maxlen - 1) {
		c = getchar();
		if (c == EOF) break;
		buffer[i++] = (char)c;
		if (c == '\n') break;
	}

	buffer[i] = '\0'; // Null-terminate

	return (i > 0) ? buffer : NULL;
}



// Bit-bang SPI transfer
uint8_t spi_transfer(uint8_t data) {
	uint8_t received = 0;
	for (int i = 7; i >= 0; i--) {
		SET_MOSI((data >> i) & 1);
		SET_SCLK(1);
		received <<= 1;
		if (READ_MISO()) received |= 1;
		SET_SCLK(0);
	}
	return received;
}

void sd_send_command(uint8_t cmd, uint32_t arg, uint8_t crc) {
	//printf("Sending CMD%u with arg: 0x%08X and CRC: 0x%02X\n", cmd, arg, crc);  // Log command and arguments 
	spi_transfer(0x40 | cmd);
	spi_transfer((arg >> 24) & 0xFF);
	spi_transfer((arg >> 16) & 0xFF);
	spi_transfer((arg >> 8) & 0xFF);
	spi_transfer(arg & 0xFF);
	spi_transfer(crc);
}

uint8_t sd_wait_r1(void) {
	for (int i = 0; i < 8; i++) {
		uint8_t r = spi_transfer(0xFF);
		if ((r & 0x80) == 0) {
			// printf("R1 response: 0x%02X\n", r);	// Log R1 response
			return r;
		}
	}
	return 0xFF;
}

int sd_read_block(uint32_t sector, uint8_t *buffer) {
	uint32_t addr = card_is_sdhc ? sector : sector * 512;
	SET_CS(0);

	sd_send_command(CMD17, addr, 0xFF);
	uint8_t r1 = sd_wait_r1();

	if (r1 != 0x00) {
		printf("CMD17 failed: 0x%02X\n", r1);
		SET_CS(1);
		spi_transfer(0xFF);
		return -1;
	}

	for (int i = 0; i < 10000; i++) {
		uint8_t token = spi_transfer(0xFF);
		if (token == 0xFE) break;
		if (i == 9999) {
			printf("Timeout waiting for data token\n");
			SET_CS(1);
			return -1;
		}
	}

	for (int i = 0; i < 512; i++) {
		buffer[i] = spi_transfer(0xFF);
	}

	// Optional: print first 16 bytes of the read block for debugging
	/*printf("Read block (first 16 bytes): ");
	for (int i = 0; i < 16; i++) {
		printf("%02X ", buffer[i]);
	}
	printf("\n"); 
	*/

	spi_transfer(0xFF);
	spi_transfer(0xFF);

	SET_CS(1);
	spi_transfer(0xFF);
	return 0;
}


int sd_read_sectors(uint32_t sector, uint8_t *buffer, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		if (sd_read_block(sector + i, buffer + i * 512) != 0) {
			return -1; // Error reading block
		}
	}
	return 0; // Success
}

int sd_write_block(uint32_t sector, const uint8_t *buffer) {
	uint32_t addr = (card_is_sdhc) ? sector : sector * 512;

	SET_CS(0);	// Enable CS

	sd_send_command(CMD24, addr, 0xFF);
	uint8_t r1 = sd_wait_r1();
	if (r1 != 0x00) {
		SET_CS(1);
		spi_transfer(0xFF);	 // Clock out
		printf("CMD24 failed with R1: 0x%02X\n", r1);
		return -1;
	}

	spi_transfer(0xFE);	 // Start data token for single block write

	// Send 512 bytes
	for (int i = 0; i < 512; i++) {
		spi_transfer(buffer[i]);
	}

	// Send dummy CRC (two bytes, required even in SPI mode)
	spi_transfer(0xFF);
	spi_transfer(0xFF);

	// Read data response token
	uint8_t response = spi_transfer(0xFF);
	if ((response & 0x1F) != 0x05) {
		SET_CS(1);
		spi_transfer(0xFF);
		printf("Data rejected with response: 0x%02X\n", response);
		return -2;
	}

	// Wait for write to complete (busy wait while MISO is 0)
	while (spi_transfer(0xFF) == 0x00);

	SET_CS(1);	// Disable CS
	spi_transfer(0xFF);	 // Extra clock

	return 0;
}


int sd_write_sectors(uint32_t sector, const uint8_t *buffer, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		int result = sd_write_block(sector + i, buffer + i * 512);
		if (result != 0) {
			printf("Error writing sector %u\n", sector + i);
			return result;	// Return on first error
		}
	}
	return 0;  // Success
}

int sd_init(void) {
	const int max_attempts = 5;

	// Initialize lines: set idle state
	SET_CS(1);
	SET_MOSI(1);
	SET_SCLK(0);

	for (int attempt = 1; attempt <= max_attempts; ++attempt) {
		printf("Initializing SD card (attempt %d of %d)...\n", attempt, max_attempts);

		SET_CS(1);
		for (int i = 0; i < 10; i++) spi_transfer(0xFF);
		delay_ms(1);

		int cmd0_success = 0;
		for (int tries = 0; tries < 10; tries++) {
			SET_CS(0);
			sd_send_command(CMD0, 0, 0x95);
			uint8_t r1 = sd_wait_r1();
			SET_CS(1);
			spi_transfer(0xFF);
			if (r1 == 0x01) {
				cmd0_success = 1;
				break;
			}
			delay_ms(1);
		}

		if (!cmd0_success) {
			printf("CMD0 failed to enter IDLE state\n");
			delay_ms(5);
			continue;
		}

		delay_ms(10);

		SET_CS(0);
		sd_send_command(CMD8, 0x1AA, 0x87);
		uint8_t r1 = sd_wait_r1();
		uint8_t cmd8_resp[4];
		int i;
		for (i = 0; i < 4; i++) cmd8_resp[i] = spi_transfer(0xFF);
		SET_CS(1);
		spi_transfer(0xFF);

		printf("CMD8 response: %02X %02X %02X %02X\n", cmd8_resp[0], cmd8_resp[1], cmd8_resp[2], cmd8_resp[3]);

		if (r1 == 0x01 && cmd8_resp[2] == 0x01 && cmd8_resp[3] == 0xAA) {
			// SD v2
			for (i = 0; i < 1000; i++) {
				SET_CS(0);
				sd_send_command(CMD55, 0, 0x65);
				uint8_t r = sd_wait_r1();
				SET_CS(1);
				spi_transfer(0xFF);

				if (r > 1) continue;

				SET_CS(0);
				 (CMD41, 0x40000000, 0x77);
				r = sd_wait_r1();
				SET_CS(1);
				spi_transfer(0xFF);

				if (r == 0x00) {
					SET_CS(0);
					sd_send_command(CMD58, 0, 0xFD);
					r = sd_wait_r1();
					uint8_t ocr[4];
					for (int i = 0; i < 4; i++) ocr[i] = spi_transfer(0xFF);
					SET_CS(1);
					spi_transfer(0xFF);

					printf("OCR: %02X %02X %02X %02X\n", ocr[0], ocr[1], ocr[2], ocr[3]);

					if (ocr[0] & 0x40) {
						printf("SDHC/SDXC card detected\n");
						card_is_sdhc = 1;
					} else {
						printf("Standard capacity SD card detected\n");
						card_is_sdhc = 0;
					}

					printf("Card initialized (SD v2)\n");
					return 0;
				}
				delay_ms(1);
			}

			printf("ACMD41 timeout (SD v2)\n");

		} else {
			// SD v1
			for (int i = 0; i < 2000; i++) {
				SET_CS(0);
				sd_send_command(CMD1, 0, 0xF9);
				r1 = sd_wait_r1();
				SET_CS(1);
				spi_transfer(0xFF);

				if (r1 == 0x00) {
					card_is_sdhc = 0;
					printf("Card initialized (SD v1)\n");
					return 0;
				}
				delay_ms(1);
			}

			printf("CMD1 timeout (SD v1)\n");
		}

		printf("Retrying SD card init...\n");
		delay_ms(10);
	}

	printf("SD card initialization failed after %d attempts\n", max_attempts);
	return -1;
}

void list_files(void) {
	FRESULT res;
	FILINFO fno;
	DIR dir;

#if _USE_LFN
	// Allocate buffer for long filename
	static char lfn[_MAX_LFN + 1];
	fno.lfname = lfn;
	fno.lfsize = sizeof(lfn);
#endif

	printf("Listing files in: %s\n", sd_path);
	res = f_opendir(&dir, sd_path);
	if (res != FR_OK) {
		printf("Failed to open directory: %d\n", res);
		return;
	}

	for (;;) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0) break;

#if _USE_LFN
		if (*fno.lfname) {
			printf("%s%s\n", (fno.fattrib & AM_DIR) ? "[DIR] " : "		", fno.lfname);
		} else
#endif
		{
			printf("%s%s\n", (fno.fattrib & AM_DIR) ? "[DIR] " : "		", fno.fname);
		}
	}

	f_closedir(&dir);
}


void view_file(void) {
	char filename[128];
	printf("Enter filename to view: ");
	scanf(filename, sizeof(filename));	// Call the custom scanf function

	FIL file;
	FRESULT res = f_open(&file, filename, FA_READ);
	if (res != FR_OK) {
		printf("f_open error: %d\n", res);
		return;
	}

	char buffer[128];
	UINT br;
	printf("Contents of %s:\n", filename);
	while ((res = f_read(&file, buffer, sizeof(buffer) - 1, &br)) == FR_OK && br > 0) {
		buffer[br] = '\0';
		printf("%s", buffer);
	}
	f_close(&file);
	printf("\n");
}

void copy_to_ram(void) {
	printf("Copied SD file to RAM");


}

void rename_file(void) {
	char oldname[128], newname[128];
	printf("Enter current filename: ");
	scanf(oldname, sizeof(oldname));
	printf("Enter new filename: ");
	scanf(newname, sizeof(newname));

	FRESULT res = f_rename(oldname, newname);
	if (res != FR_OK) {
		printf("f_rename error: %d\n", res);
	} else {
		printf("Renamed '%s' to '%s'\n", oldname, newname);
	}
}

void delete_file(void) {
	char filename[128];
	printf("Enter filename to delete: ");
	scanf(filename, sizeof(filename));

	FRESULT res = f_unlink(filename);
	if (res != FR_OK) {
		printf("f_unlink error: %d\n", res);
	} else {
		printf("Deleted file: %s\n", filename);
	}
}

void create_file(void) {
	char filename[128];
	printf("Enter new filename to create: ");
	scanf(filename, sizeof(filename));

	FIL file;
	FRESULT res = f_open(&file, filename, FA_CREATE_NEW | FA_WRITE);
	if (res == FR_EXIST) {
		printf("File already exists: %s\n", filename);
	} else if (res != FR_OK) {
		printf("f_open error: %d\n", res);
	} else {
		printf("Created empty file: %s\n", filename);
		f_close(&file);
	}
}

void change_directory(void) {
	char new_dir[128];
	printf("Enter directory to change to (relative or absolute): ");
	scanf(new_dir, sizeof(new_dir));

	char test_path[256];
	if (new_dir[0] == '/') {
		// Absolute path from SD root
		printf(test_path, sizeof(test_path), "0:%s", new_dir);
	} else {
		// Relative to current path
		printf(test_path, sizeof(test_path), "%s/%s", sd_path, new_dir);
	}

	DIR dir;
	FRESULT res = f_opendir(&dir, test_path);
	if (res != FR_OK) {
		printf("Failed to open directory: %d\n", res);
		return;
	}

	strncpy(sd_path, test_path, sizeof(sd_path));
	f_closedir(&dir);
	printf("Changed SD card directory to: %s\n", sd_path);
}

void create_directory() {
	char path[256];
	printf("Enter name of directory to create: ");
	fgets(path, sizeof(path));
	path[strcspn(path, "\r\n")] = 0;  // Remove newline

	FRESULT res = f_mkdir(path);
	if (res == FR_OK) {
		printf("Directory created successfully.\n");
	} else {
		printf("Failed to create directory: %d\n", res);
	}
}

void delete_directory() {
	char path[256];
	printf("Enter name of directory to delete: ");
	fgets(path, sizeof(path));
	path[strcspn(path, "\r\n")] = 0;  // Remove newline

	FRESULT res = f_unlink(path);
	if (res == FR_OK) {
		printf("Directory deleted successfully.\n");
	} else {
		printf("Failed to delete directory: %d\n", res);
	}
}

void print_current_directory(void) {
	printf("Current SD directory: %s\n", sd_path);
}

void menu(void) {
	int choice;

	do {
		printf("\nSD Card File Manager - ");
		print_current_directory();
		printf("1. List Files in the SD Card\n");
		printf("2. View File Contents\n");
		printf("3. Copy File from SD to RAM\n");
		printf("4. Rename a File in the SD Card\n");
		printf("5. Delete a File in the SD Card\n");
		printf("6. Create a New Empty File on the SD Card\n");
		printf("7. Change Directory on SD Card\n");
		printf("8. Print Current Directory on SD Card\n");
		printf("9. Create Directory on SD Card\n");
		printf("10. Delete Directory on SD Card\n");
		printf("0. Exit\n");
		printf("Enter your choice: ");
		scanf(&choice, sizeof(choice));
		getchar(); // consume newline

		switch (choice) {
			case 1: list_files(); break;
			case 2: view_file(); break;
			case 3: copy_to_ram(); break;
			case 4: rename_file(); break;
			case 5: delete_file(); break;
			case 6: create_file(); break;
			case 7: change_directory(); break;
			case 8: print_current_directory(); break;
			case 9: create_directory(); break;
			case 10: delete_directory(); break;
			case 0: break;
			default: printf("Invalid choice.\n");
		}
	} while (choice != 0);
}
// Function to read and print a file using FatFs
int fatfs_read_file(const char *filename) {
	FATFS fs;
	FIL file;
	FRESULT res;
	UINT br;
	char buffer[128];

	res = f_mount(&fs, "", 1);
	if (res != FR_OK) {
		printf("f_mount error: %d\n", res);
		return -1;
	}

	res = f_open(&file, filename, FA_READ);
	if (res != FR_OK) {
		printf("f_open error: %d\n", res);
		return -1;
	}

	printf("Reading file: %s\n", filename);
	while ((res = f_read(&file, buffer, sizeof(buffer) - 1, &br)) == FR_OK && br > 0) {
		buffer[br] = '\0';
		printf("%s", buffer);
	}

	f_close(&file);
	f_mount(NULL, "", 1);
	return 0;
}

int main(void) {
 
	printf("SD card initialization starting...\n");

	if (sd_init() == 0) {
		printf("SD card successfully initialized!\n");
	} else {
		printf("SD card initialization failed\n");
	}

	FRESULT res = f_mount(&fs, "", 1);
	if (res != FR_OK) {
		printf("f_mount failed: %d\n", res);
		return -1;
	}

	menu();

	f_mount(NULL, "", 1);

	return 0;
}

