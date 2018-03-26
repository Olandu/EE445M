// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Jonathan W. Valvano 3/9/17
// Method: FAT (File Allocation Table) --> 1-to-1 mapping of the disk

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "edisk.h"
#include "UART2.h"
#include "OS.h"

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

uint8_t RAM[BLOCK_SIZE], FAT[BLOCK_SIZE], idxWrite_OpenFile = 0, idxRead_OpenFile = 0; // 0 if none open else index into the FAT directory
int nextByteRead = 0;

int BytesWritten[NUM_SECTOR]; 

Sema4Type Read, Write;


struct FATdirectory{ 					// Size: (NAME + 1 + 1) bytes
	BYTE name[NAME];
	BYTE startSector;
	BYTE size;
};
typedef struct FATdirectory dType;
dType directory[NUM_FILES];

//----------Helper functions------------
void convert_Dir2Buff (void){
	int i, j, numFile = 0;
	for (i = 0; i < BLOCK_SIZE; i += DIR_SIZE){
		for (j = 0; j < NAME; j++){
			RAM[i + j] = directory[numFile].name[j];
		}
		RAM[i+ NAME] = directory[numFile].startSector;
		RAM[i+ (NAME + 1)] = directory[numFile].size;
		numFile++;
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
	int i, j, numFile = 0;
	
	// Read directory into the dataBuff array
	if (eDisk_ReadBlock(RAM, dir)) 
		return FAIL;
	
	
	for (i = 0; i < BLOCK_SIZE; i += DIR_SIZE){
		for (j = 0; j < NAME; j++){
			directory[numFile].name[j] = RAM[i+j];
		}
		directory[numFile].startSector = RAM[i+NAME];
		directory[numFile].size = RAM[i+(NAME+1)];
		numFile++;
	}
	
	return SUCCESS;
}

int readFAT(void){
	// Read FAT
	if (eDisk_ReadBlock(FAT, FAT_SECTOR)) 
		return FAIL;
	return SUCCESS;
}

int compareName(char name[], int length){
	int currDir_idx, i;
	char dirName[NAME]; 
	for (currDir_idx = 1; currDir_idx < NUM_FILES; currDir_idx++){
		// compare names, if matches, then break
		for(i = 0; i < NAME; i++){
			dirName[i] = directory[currDir_idx].name[i];
		}
		if(strncmp(name, dirName, strlen(name)) == 0){
			break;
		}
	}
	return currDir_idx;
}

int allocateBlock(int file_idx, int lastSector){
	int freeStartSector_idx;
	
	//No free space in Directory
	if (directory[FREE].size <= 0) 
		return FAIL;	
	
	freeStartSector_idx = directory[FREE].startSector;
	
	// assign a sector to the new file (one block only)
	if (lastSector == 0){
		directory[file_idx].startSector = freeStartSector_idx;
	} else {
		FAT[lastSector] = freeStartSector_idx;
	}
	BytesWritten[freeStartSector_idx] = 0;
	directory[file_idx].size++;
	
	//new free start sector
	directory[FREE].startSector = FAT[freeStartSector_idx]; 
	FAT[freeStartSector_idx] = NULL;									// points to Null
	
	directory[FREE].size--; //should not be less than zero
	
	return SUCCESS;
}

int findLastSector(int file_idx){
	int startSector, lastSector;
	
	startSector = directory[file_idx].startSector;
	if(FAT[startSector] == 0) return startSector;
	
	lastSector = FAT[startSector];
	while (FAT[lastSector] > 0){
		lastSector = FAT[lastSector];
	}
	
	return lastSector;
}

int readData(int dataSector){
	if (eDisk_ReadBlock(RAM, dataSector)) 
		return FAIL;

	return SUCCESS;
}

int writeData(int dataSector){
	if(eDisk_WriteBlock(RAM, dataSector)) 
		return FAIL;

	return SUCCESS;
}

//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system
	if(eDisk_Init(0)) 
		return FAIL;
  OS_InitSemaphore(&Read,1);  // means Read free
	OS_InitSemaphore(&Write,1);  // 
	return SUCCESS;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){ // erase disk, add format
	int i =0, j = 0;
	// Initialize Free_space field in the directory (first entry)
	directory[FREE].name[0] = '*';
	directory[FREE].size = FREE_SECTORS;	
	directory[FREE].startSector = 2;
	
	for(i = 1; i < NUM_FILES; i++){
		for(j =0; j < NAME; j++){
			directory[i].name[j] = 0;
		}
		directory[i].size = 0;	
		directory[i].startSector = 0;
	}		
	
	// Link free space in FAT
	for (i = 2; i < FREE_SPACE; i++) {
		FAT[i] = i + 1;
	}
	FAT[FREE_SPACE] = NULL; // null pointer
	
	// Number of Bytes written in each data sector
	for (i = 0; i < NUM_SECTOR; i++){
		BytesWritten[i] = 0;
	}
	
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
	int i,freeFile_idx;
	
	readDIR(DIR_SECTOR);
	readFAT();
	
	//No free space in Directory
	if (directory[0].size < 1) 
		return FAIL;					
	
	for (freeFile_idx = 1; freeFile_idx < NUM_FILES; freeFile_idx++){
		if (directory[freeFile_idx].name[0] == 0) break;
	}
	
	//File name is too long
	if (strlen(name) > NAME) 
		return FAIL;
	
	// write fileName to the directory
	for (i = 0; i < NAME; i++){
		directory[freeFile_idx].name[i] = name[i];
	}
	
	allocateBlock(freeFile_idx, 0); 
	
	// Writing the directory and the FAT to the disk;
	convert_Dir2Buff ();
	if (write_DirFAT()) return FAIL;
	
  return SUCCESS;     
}

//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){      // open a file for writing 
	int dirIdx, lastSector, length;
	
	OS_Wait(&Write);
	readDIR(DIR_SECTOR);
	readFAT();
	
	length = strlen(name);
	if ((length > NAME) || !length)
		return FAIL;
	
	dirIdx = compareName(name, length);
	if(dirIdx == 0) return FAIL;
	
	lastSector = findLastSector(dirIdx);
	
	if (lastSector == 0) return FAIL;
	
	readData(lastSector);
	// read the last block of the given file
	idxWrite_OpenFile = dirIdx;
  return SUCCESS;   
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write(char data){
	int lastSector;
	
	//we should be able to read a file within a write but not the other way around***
	if (idxWrite_OpenFile == 0)
		return FAIL; 
	
	// Find the last sector of the open file
	lastSector = findLastSector(idxWrite_OpenFile);
	
	
	readData(lastSector);
	
	if(BytesWritten[lastSector] == BLOCK_SIZE) { // allocate a new block/sector
			if (allocateBlock(idxWrite_OpenFile, lastSector)) 
				return FAIL; 
			lastSector = FAT[lastSector];	
	}
		
	RAM[BytesWritten[lastSector]] = data;
	BytesWritten[lastSector]++;

	if(writeData(lastSector))
		return FAIL;
	
	// Writing the directory and the FAT to the disk
	convert_Dir2Buff ();
	if (write_DirFAT()) return FAIL;
		
  return SUCCESS;  
}

int eFile_WClose(void);
int eFile_RClose(void);

//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){ 
	if (write_DirFAT()) return FAIL;
  
	return SUCCESS;     
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){ // close the file for writing
	if (!idxWrite_OpenFile) return FAIL;
	idxWrite_OpenFile = 0;
  OS_Signal(&Write);
  return SUCCESS;     
}


//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen(char name[]){      // open a file for reading 
	int dir_idx = 0, firstSector = 0;
	
	OS_Wait(&Read);
	OS_Wait(&Write);
//	ReadCount ++;
//	if(ReadCount == 1) OS_Wait(&Write);
		
	readDIR(DIR_SECTOR);
	readFAT();
	
	if(strlen(name) > NAME)
		return FAIL;
	dir_idx = compareName(name, strlen(name));
	if(dir_idx == 0)
		return FAIL;
	firstSector = directory[dir_idx].startSector;
	if(firstSector < 2) // 0-->directory; 1-->FATTable (mapping to disk)
		return FAIL;
	if(eDisk_ReadBlock(RAM, firstSector))
		return FAIL;
	idxRead_OpenFile = firstSector;
	
//	OS_bSignal(&ReadMutex);
	
  return SUCCESS;     
}
 
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext(char *pt){       // get next byte 

	if(idxRead_OpenFile == 0)
		return FAIL;
	
	if(BytesWritten[idxRead_OpenFile] == 0)
		return FAIL; 
		
	readData(idxRead_OpenFile);
	
	if(nextByteRead == BLOCK_SIZE){
		idxRead_OpenFile = FAT[idxRead_OpenFile];
		if(eDisk_ReadBlock(RAM, idxRead_OpenFile))
			return FAIL;
		nextByteRead = 0;
	}
	*pt = RAM[nextByteRead];
	nextByteRead++;

  return SUCCESS; 
}

    
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){ // close the file for writing
//	OS_bWait(&ReadMutex);
//	ReadCount--;
//	if(ReadCount == 0)
//		OS_Signal(&Write);
	
	if(idxRead_OpenFile == 0)
		return FAIL;
	
	idxRead_OpenFile = 0;
	nextByteRead = 0;
	
	OS_Signal(&Write);
	OS_Signal(&Read);
  return SUCCESS;
}




//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: none
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(void(*fp)(char)){  
	int i, j;
	if(readDIR(DIR_SECTOR))
		return FAIL;
	for(i = 1; i < NUM_FILES; i++){ 
		if(directory[i].name[0] != 0){//skip free space
			for(j = 0; j < NAME; j++){
				fp((char)directory[i].name[j]);
			}
			fp('\t');
			fp(directory[i].size + 0x30);
			fp('\n');
			fp('\r');
		}
	}
  return SUCCESS;
}

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){  // remove this file 
	int i, dirIdx = 0, sector = 0, lastFreeSector = 0, length = strlen(name);
	
	readDIR(DIR_SECTOR);
	readFAT();
	
	if(strlen(name) > NAME)
		return FAIL;
	
	dirIdx = compareName(name, length);
	if((dirIdx == 0)||(dirIdx == NUM_FILES)) return FAIL;
	
	lastFreeSector = findLastSector(FREE);
	
	// Free space
	FAT[lastFreeSector] = directory[dirIdx].startSector;
	
	sector = directory[dirIdx].startSector;
	for (i = 0; i < directory[dirIdx].size; i++){
		BytesWritten[sector] = 0;
		sector = FAT[sector];
	}
	
	directory[dirIdx].name[0] = 0;
	directory[dirIdx].size = 0;
	directory[dirIdx].startSector = 0;
	
	// Writing the directory and the FAT to the disk
	convert_Dir2Buff ();
	if (write_DirFAT()) return FAIL;

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
