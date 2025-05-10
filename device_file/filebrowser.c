/*---------------------------------------------------------------*/
/* Petit-FatFs module test program R0.03a (C)ChaN, 2019          */
/*---------------------------------------------------------------*/

#include "stdio.h"
#include "../FatFS/source/ff.h"
#include "../FatFS/source/diskio.h"

/*-----------------------------------------------------------------------*/
/* Main                                                                  */

void showMenu(void) {
	printf("\t\tFatFS File System Browser\n");
	printf("\t=================================\n");
	printf("\t1 - List Files\n");
	printf("\t2 - Change Dir\n");
	printf("\t3 - Show Text File Content\n");
	printf("\t4 - Initialize Disk\n");
	printf("\t5 - Format Disk as Fat16\n");
	printf("\t6 - Format Disk as Fat132\n");
	printf("\n\tQ - Quit the program\n\n");	
}

char readKey() {
    char choice;
    
    // Prompt the user for input
    printf("Please choose an option: ");
    
    // Read a single character input from the user
    choice = getchar();

    // To consume the newline character left behind by pressing Enter
    getchar(); 
    
    return choice;
}

// Function declarations for each menu option
void listFiles() {
    printf("Listing files...\n");
}

void changeDir() {
    printf("Changing directory...\n");
}

void showTextFileContent() {
    printf("Showing text file content...\n");
}

void formatDiskFat16() {
    printf("Formatting disk as FAT16...\n");
}

void formatDiskFat132() {
    printf("Formatting disk as FAT132...\n");
}

int main (void)
{

	char *ptr;
	long p1, p2;
	BYTE res;
	UINT s1, s2, s3, ofs, cnt, w;
	FATFS fs;
	DIR dir;
	FILINFO fno;

	char userChoice;
	
	showMenu();
	userChoice = readKey();
    // Use switch statement to call functions based on user input
    switch(userChoice) {
        case '1':
            listFiles();
            break;
        case '2':
            changeDir();
            break;
        case '3':
            showTextFileContent();
            break;
        case '4':
		    BYTE pdrv = 3;
    		DSTATUS result = disk_initialize(pdrv);
    
    		if (result == RES_OK) {
        		printf("Disk %u initialized successfully.\n", pdrv);
    		} else {
        		printf("Failed to initialize disk %u.\n", pdrv);
    		}
            break;
        case '5':
            formatDiskFat16();
            break;
        case '6':
            formatDiskFat132();
            break;
        case 'Q':
        case 'q':
            printf("Quitting the program...\n");
            break;
        default:
            printf("Invalid option. Please try again.\n");
    }	

}
