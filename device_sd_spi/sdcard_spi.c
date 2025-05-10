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
#include <unistd.h>
#include <pigpio.h>
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


static int card_is_sdhc = 0;
static int spi_handle = -1;
FATFS fs;
char sd_path[256] = "0:";  // Initial SD root directory

uint8_t spi_transfer(uint8_t data) {
	char tx = data;
	char rx;
	spiXfer(spi_handle, &tx, &rx, 1);  // Full-duplex SPI transfer
	return (uint8_t)rx;
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
	gpioWrite(CS_PIN, 0);
	
	//uint32_t addr = block_addr * 512;
	uint32_t addr = (card_is_sdhc) ? sector : sector * 512;
	
	sd_send_command(CMD17, addr, 0xFF);
	uint8_t r1 = sd_wait_r1();

	if (r1 != 0x00) {
		printf("CMD17 failed: 0x%02X\n", r1);
		gpioWrite(CS_PIN, 1);
		spi_transfer(0xFF);
		return -1;
	}

	for (int i = 0; i < 10000; i++) {
		uint8_t token = spi_transfer(0xFF);
		if (token == 0xFE) break;
		usleep(10);
		if (i == 9999) {
			printf("Timeout waiting for data token\n");
			gpioWrite(CS_PIN, 1);
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

	gpioWrite(CS_PIN, 1);
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

	gpioWrite(CS_PIN, 0);  // Enable CS

	sd_send_command(CMD24, addr, 0xFF);
	uint8_t r1 = sd_wait_r1();
	if (r1 != 0x00) {
		gpioWrite(CS_PIN, 1);
		spi_transfer(0xFF);	 // Clock out
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
		gpioWrite(CS_PIN, 1);
		spi_transfer(0xFF);
		return -2;	// Data rejected
	}

	// Wait for write to finish
	while (spi_transfer(0xFF) == 0x00);	 // Busy wait

	gpioWrite(CS_PIN, 1);  // Disable CS
	spi_transfer(0xFF);	   // Extra clock

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

	// Use only CS as GPIO, SPI will handle SCLK/MOSI/MISO
	gpioSetMode(CS_PIN, PI_OUTPUT);
	gpioWrite(CS_PIN, 1);  // CS inactive (high)

	// Open SPI channel 0 at 400 kHz for init phase
	spi_handle = spiOpen(0, 400000, 0);	 // SPI0, 400kHz, mode 0
	if (spi_handle < 0) {
		printf("Failed to open SPI\n");
		return -1;
	}

	for (int attempt = 1; attempt <= max_attempts; ++attempt) {
		printf("Initializing SD card (attempt %d of %d)...\n", attempt, max_attempts);

		gpioWrite(CS_PIN, 1);
		for (int i = 0; i < 10; i++) spi_transfer(0xFF);
		usleep(1000);

		int cmd0_success = 0;
		for (int tries = 0; tries < 10; tries++) {
			gpioWrite(CS_PIN, 0);
			sd_send_command(CMD0, 0, 0x95);
			uint8_t r1 = sd_wait_r1();
			gpioWrite(CS_PIN, 1);
			spi_transfer(0xFF);
			if (r1 == 0x01) {
				cmd0_success = 1;
				break;
			}
			usleep(1000);
		}

		if (!cmd0_success) {
			printf("CMD0 failed to enter IDLE state\n");
			usleep(5000);
			continue;
		}

		usleep(10000);

		gpioWrite(CS_PIN, 0);
		sd_send_command(CMD8, 0x1AA, 0x87);
		uint8_t r1 = sd_wait_r1();
		uint8_t cmd8_resp[4];
		for (int i = 0; i < 4; i++) cmd8_resp[i] = spi_transfer(0xFF);
		gpioWrite(CS_PIN, 1);
		spi_transfer(0xFF);

		printf("CMD8 response: %02X %02X %02X %02X\n", cmd8_resp[0], cmd8_resp[1], cmd8_resp[2], cmd8_resp[3]);

		if (r1 == 0x01 && cmd8_resp[2] == 0x01 && cmd8_resp[3] == 0xAA) {
			for (int i = 0; i < 1000; i++) {
				gpioWrite(CS_PIN, 0);
				sd_send_command(CMD55, 0, 0x65);
				uint8_t r = sd_wait_r1();
				gpioWrite(CS_PIN, 1);
				spi_transfer(0xFF);

				if (r > 1) continue;

				gpioWrite(CS_PIN, 0);
				sd_send_command(CMD41, 0x40000000, 0x77);
				r = sd_wait_r1();
				gpioWrite(CS_PIN, 1);
				spi_transfer(0xFF);

				if (r == 0x00) {
					gpioWrite(CS_PIN, 0);
					sd_send_command(CMD58, 0, 0xFD);
					r = sd_wait_r1();
					uint8_t ocr[4];
					for (int i = 0; i < 4; i++) ocr[i] = spi_transfer(0xFF);
					gpioWrite(CS_PIN, 1);
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
				usleep(1000);
			}
			printf("ACMD41 timeout (SD v2)\n");
		} else {
			for (int i = 0; i < 2000; i++) {
				gpioWrite(CS_PIN, 0);
				sd_send_command(CMD1, 0, 0xF9);
				r1 = sd_wait_r1();
				gpioWrite(CS_PIN, 1);
				spi_transfer(0xFF);

				if (r1 == 0x00) {
					printf("Card initialized (SD v1)\n");
					return 0;
				}
				usleep(1000);
			}
			printf("CMD1 timeout (SD v1)\n");
		}

		printf("Retrying SD card init...\n");
		usleep(10000);
	}

	printf("SD card initialization failed after %d attempts\n", max_attempts);
	return -1;
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
	scanf("%127s", filename);

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

void copy_to_local(void) {
	char sd_filename[128], local_path[256];
	printf("Enter SD file to copy from: ");
	scanf("%127s", sd_filename);
	printf("Enter destination path (local): ");
	scanf("%255s", local_path);

	FIL sd_file;
	FRESULT res = f_open(&sd_file, sd_filename, FA_READ);
	if (res != FR_OK) {
		printf("f_open error: %d\n", res);
		return;
	}

	FILE *out = fopen(local_path, "wb");
	if (!out) {
		printf("Failed to open local file: %s\n", local_path);
		f_close(&sd_file);
		return;
	}

	char buffer[512];
	UINT br;
	while (f_read(&sd_file, buffer, sizeof(buffer), &br) == FR_OK && br > 0) {
		fwrite(buffer, 1, br, out);
	}

	printf("Copied SD card file '%s' to local file '%s'\n", sd_filename, local_path);
	f_close(&sd_file);
	fclose(out);
}

void copy_from_local(void) {
	char local_path[256], sd_filename[128];
	printf("Enter source path (local file): ");
	scanf(" %255s", local_path);
	printf("Enter destination filename on SD card (e.g., 0:/file.txt): ");
	scanf(" %127s", sd_filename);

	FILE *in = fopen(local_path, "rb");
	if (!in) {
		printf("Failed to open local file: %s\n", local_path);
		return;
	}

	FIL sd_file;
	FRESULT res = f_open(&sd_file, sd_filename, FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK) {
		printf("Failed to open SD card file '%s': f_open error %d\n", sd_filename, res);
		fclose(in);
		return;
	}

	char buffer[512];
	size_t read_bytes;
	UINT written_bytes;
	int error_occurred = 0;

	while ((read_bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
		res = f_write(&sd_file, buffer, read_bytes, &written_bytes);
		if (res != FR_OK || written_bytes != read_bytes) {
			printf("Write error: f_write returned %d, bytes written = %u\n", res, written_bytes);
			error_occurred = 1;
			break;
		}
	}

	fclose(in);
	f_close(&sd_file);

	if (!error_occurred) {
		printf("Successfully copied '%s' to SD card as '%s'\n", local_path, sd_filename);
	} else {
		printf("An error occurred while copying '%s' to '%s'\n", local_path, sd_filename);
	}
}


void rename_file(void) {
	char oldname[128], newname[128];
	printf("Enter current filename: ");
	scanf("%127s", oldname);
	printf("Enter new filename: ");
	scanf("%127s", newname);

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
	scanf("%127s", filename);

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
	scanf("%127s", filename);

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
	scanf("%127s", new_dir);

	char test_path[256];
	if (new_dir[0] == '/') {
		// Absolute path from SD root
		snprintf(test_path, sizeof(test_path), "0:%s", new_dir);
	} else {
		// Relative to current path
		snprintf(test_path, sizeof(test_path), "%s/%s", sd_path, new_dir);
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
	fgets(path, sizeof(path), stdin);
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
	fgets(path, sizeof(path), stdin);
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
		printf("3. Copy File from SD to local filesystem\n");
		printf("4. Copy File from local filesystem to the SD Card\n");
		printf("5. Rename a File in the SD Card\n");
		printf("6. Delete a File in the SD Card\n");
		printf("7. Create a New Empty File on the SD Card\n");
		printf("8. Change Directory on SD Card\n");
		printf("9. Print Current Directory on SD Card\n");
		printf("10. Create Directory on SD Card\n");
		printf("11. Delete Directory on SD Card\n");
		printf("0. Exit\n");
		printf("Enter your choice: ");
		scanf("%d", &choice);
		getchar(); // consume newline

		switch (choice) {
			case 1: list_files(); break;
			case 2: view_file(); break;
			case 3: copy_to_local(); break;
			case 4: copy_from_local(); break;
			case 5: rename_file(); break;
			case 6: delete_file(); break;
			case 7: create_file(); break;
			case 8: change_directory(); break;
			case 9: print_current_directory(); break;
			case 10: create_directory(); break;
			case 11: delete_directory(); break;
			case 0: break;
			default: printf("Invalid choice.\n");
		}
	} while (choice != 0);
}


int main(void) {
	if (gpioInitialise() < 0) {
		printf("pigpio initialization failed\n");
		return -1;
	}

	if (sd_init() != 0) {
		printf("SD card initialization failed\n");
		gpioTerminate();
		return -1;
	}

	FRESULT res = f_mount(&fs, "", 1);
	if (res != FR_OK) {
		printf("f_mount failed: %d\n", res);
		gpioTerminate();
		return -1;
	}

	menu();

	f_mount(NULL, "", 1);
	gpioTerminate();
	return 0;
}


