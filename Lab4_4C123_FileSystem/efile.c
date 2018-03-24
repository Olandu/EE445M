// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Jonathan W. Valvano 3/9/17
// Method: FAT (File Allocation Table) --> 1-to-1 mapping of the disk

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
#define NUM_FILES 64 					// 512 bytes / 8 bytes (size of the FATdirectory struct)
#define FREE_SPACE 255
#define FREE_SECTORS	FREE_SPACE-2 // at the time of format
#define DIR_SECTOR 0					// secotr 0 contains the directory in the disk
#define FAT_SECTOR 1					// sector 1 contains the FAT in the disk
#define FREE 0
#define NULL 0
#define DIR_SIZE 8

uint8_t RAM[BLOCK_SIZE], FAT[BLOCK_SIZE], Curr_numFiles = 0;


struct FATdirectory{ 					// Size: (NAME + 1 + 1) bytes
	BYTE name[NAME];
	BYTE startSector;
	BYTE size;
};
typedef struct FATdirectory dType;
dType directory[NUM_FILES];

//----------Helper functions------------
void convert_Dir2Buff (void){
	int i;
	for (i = 0; i < BLOCK_SIZE; i += DIR_SIZE){
		RAM[i] = directory[i].name[0];
		RAM[i+ NAME] = directory[i+NAME].size;
		RAM[i+ (NAME + 1)] = directory[i+ (NAME + 1)].startSector;
	}
}

int write_DirFAT (void){
	if (eDisk_WriteBlock(RAM, DIR_SECTOR)) 
		return FAIL; 
	if (eDisk_WriteBlock(FAT, FAT_SECTOR)) 
		return FAIL; 			
	return SUCCESS;
}

int readDIR (int dir){
	int i, j;
	
	// Read directory into the dataBuff array
	if (eDisk_ReadBlock(RAM, dir)) 
		return FAIL;
	
	for (i = 0; i < NUM_FILES; i++){
		for (j = 0; j < NAME; j++){
			directory[i].name[j] = RAM[i+j];
		}
		directory[i].startSector = RAM[i+NAME];
		directory[i].size = RAM[i+(NAME+1)];
	}
	
	return SUCCESS;
}

int readFAT(void){
	// Read FAT
	if (eDisk_ReadBlock(FAT, FAT_SECTOR)) 
		return FAIL;
	return SUCCESS;
}

int compareName(char name[]){
	int currDir_idx, i;
	char dirName[strlen(name)]; 
	for (currDir_idx = 1; currDir_idx < NUM_FILES; currDir_idx++){
		// compare names, if matches, then break
		for(i = 0; i < NAME; i++){
			dirName[i] = directory[currDir_idx].name[i];
		}
		if(strcmp(name, dirName) == 0){
			break;
		}
	}
	return currDir_idx;
}

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
		directory[FREE].name[0] = '*';
		directory[FREE].size = FREE_SECTORS;	
		directory[FREE].startSector = 2;
	
	for(i = 1; i < NUM_FILES; i++){
		directory[i].name[0] = '*';
		directory[i].size = FREE_SECTORS;	
		directory[i].startSector = 2;
	}		
	
	// Link free space in FAT
	for (i = 2; i < FREE_SPACE; i++) {
		FAT[i] = i + 1;
	}
	FAT[FREE_SPACE] = NULL; // null pointer
	
	// Writing the directory and the FAT to the disk;
	convert_Dir2Buff ();
	if (write_DirFAT()) return FAIL;
	
  return SUCCESS;   // OK
}


//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[]){  // create new file, make it empty 
	int i,freeFile_idx , freeStartSector_idx;
	
	readDIR(DIR_SECTOR);
	readFAT();
	
	//No free space in Directory
	if (directory[FREE].size < 1) 
		return FAIL;					
	
	for (freeFile_idx = 1; freeFile_idx < NUM_FILES; freeFile_idx++){
		if (directory[freeFile_idx].name[0] == '*') break;
	}
	
	//File name is too long
	if (strlen(name) > NAME) 
		return FAIL;
	
	// write fileName to the directory
	for (i = 0; i < NAME; i++){
		directory[freeFile_idx].name[i] = name[i];
	}
	
	freeStartSector_idx = directory[FREE].startSector;
	
	// assign a sector to the new file (one block only)
	directory[freeFile_idx].startSector = freeStartSector_idx;
	directory[freeFile_idx].size = 1;
	FAT[freeStartSector_idx] = NULL;									// points to Null
	
	//new free start sector
	directory[FREE].startSector = FAT[freeStartSector_idx]; 
	
	directory[FREE].size--; //should not be less than zero
	
	// Writing the directory and the FAT to the disk;
	convert_Dir2Buff ();
	if (write_DirFAT()) return FAIL;
	
	Curr_numFiles++;
  return SUCCESS;     
}

//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){      // open a file for writing 
	int dirIdx = 0, startSector = 0, lastSector = 0;
//	if (!Curr_numFiles) return FAIL;	// no files in the directory
	
	readDIR(DIR_SECTOR);
	readFAT();
	
	if(strlen(name) > NAME)
		return FAIL;
	
	dirIdx = compareName(name);
	if(dirIdx == 0) return FAIL;
	
	startSector = directory[dirIdx].startSector;
	lastSector = FAT[startSector];
	while (lastSector > 0){
		lastSector = FAT[lastSector];
	}
	readDIR(lastSector);
	// read the last block of the given file
	
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
