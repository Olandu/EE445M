// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Jonathan W. Valvano 3/9/17

#include <string.h>
#include "edisk.h"
#include "UART2.h"
#include <stdio.h>
#include <stdint.h>

#define SUCCESS 0
#define FAIL 1

#define NUM_SECTOR 256
#define BLOCK_SIZE 512
#define NAME 6
#define NUM_FILES 64 // 512 bytes / 8 bytes (struct)
#define FREE_SPACE 255

uint8_t dataBuff[BLOCK_SIZE], FAT[BLOCK_SIZE];


struct FATdirectory{
	BYTE name[NAME];
	BYTE startSector;
	BYTE size;
};
typedef struct FATdirectory dType;
dType directory[NUM_FILES];
//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system
	if(eDisk_Init(0)) 
		return FAIL;
  return SUCCESS;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){ // erase disk, add format
	int i =0;
	// Initialize Free_space field in the directory (first entry)
		directory[0].name[0] = '*';
		directory[0].size = FREE_SPACE - 3;	
		directory[0].startSector = 2;
	
	for(i = 1; i < NUM_FILES; i++){
		directory[i].name[0] = '*';
		directory[i].size = FREE_SPACE - 2;	
		directory[i].startSector = 2;
	}		
	
	// FAT
	for (i = 2; i < FREE_SPACE; i++) {
		FAT[i] = i + 1;
	}
	FAT[FREE_SPACE] = 0; // null pointer
	
	// Writing the directory to the disk;
	for (i = 0; i < BLOCK_SIZE; i++){
		dataBuff[i] = directory[i].name[0];
		dataBuff[i+6] = directory[i+6].size;
		dataBuff[i+7] = directory[i+7].startSector;
	}
	if (eDisk_WriteBlock(dataBuff, 0)) return FAIL; // sector 0 contains directory
	
	// Writing the FAT to the disk
	if (eDisk_WriteBlock(FAT, 1)) return FAIL; 	//sector 1 contains the FAT
	
  return SUCCESS;   // OK
}



//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[]){  // create new file, make it empty 
	int i, j, freeFile_idx , freeStartSector_idx, newfreeSector_idx;
	
	if (eDisk_ReadBlock(dataBuff, 0)) 
		return FAIL;
	
	// Read directory 
	for (i = 0; i < NUM_FILES; i++){
		for (j = 0; j < NAME; j++){
			directory[i].name[j] = dataBuff[i+j];
		}
		directory[i].startSector = dataBuff[i+6];
		directory[i].size = dataBuff[i+7];
	}
	
	// Read FAT
	if (eDisk_ReadBlock(FAT, 1)) 
		return FAIL;
	
	for (freeFile_idx = 1; freeFile_idx < NUM_FILES; freeFile_idx++){
		if (directory[freeFile_idx].name[0] == '*') break;
	}
	
	if (strlen(name) > NAME) 
		return FAIL;
	
	
	for (i = 0; i < NAME; i++){
		directory[freeFile_idx].name[i] = name[i];
	}
	
	if (directory[0].size < 1) 
		return FAIL;						// don't got space
	
	freeStartSector_idx = directory[0].startSector;
	
	newfreeSector_idx = FAT[freeStartSector_idx];	// new startSector for free
	
	// assign a sector to the new file
	directory[freeFile_idx].startSector = freeStartSector_idx;
	directory[freeFile_idx].size = 1;
	FAT[freeStartSector_idx] = 0;
	
	//new free start sector
	directory[0].startSector = newfreeSector_idx;
	directory[0].size--;
	
	
	//write directory and FAT to disk (use helper functions)
	
  return SUCCESS;     
}

//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){      // open a file for writing 

  return SUCCESS;   
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write(char data){

  return SUCCESS;  
}


//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){ 

  return SUCCESS;     
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){ // close the file for writing
  
  return SUCCESS;     
}


//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]){      // open a file for reading 

  return SUCCESS;     
}
 
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){       // get next byte 

  return SUCCESS; 
}

    
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){ // close the file for writing

  return SUCCESS;
}




//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: none
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(void(*fp)(char)){   

  return SUCCESS;
}

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){  // remove this file 

  return SUCCESS;    // restore directory back to flash
}

int StreamToFile=0;                // 0=UART, 1=stream to file

int eFile_RedirectToFile(char *name){
  eFile_Create(name);              // ignore error if file already exists
  if(eFile_WOpen(name)) return 1;  // cannot open file
  StreamToFile = 1;
  return 0;
}

int eFile_EndRedirectToFile(void){
  StreamToFile = 0;
  if(eFile_WClose()) return 1;    // cannot close file
  return 0;
}

int fputc (int ch, FILE *f) { 
  if(StreamToFile){
    if(eFile_Write(ch)){          // close file on error
       eFile_EndRedirectToFile(); // cannot write to file
       return 1;                  // failure
    }
    return 0; // success writing
  }

   // regular UART output
  UART_OutChar(ch);
  return 0; 
}

int fgetc (FILE *f){
  char ch = UART_InChar();  // receive from keyboard
  UART_OutChar(ch);            // echo
  return ch;
}
