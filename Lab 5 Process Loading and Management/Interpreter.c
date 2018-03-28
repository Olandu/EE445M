// *************Interpreter.c**************
// EE445M/EE380L.6 Lab 4 solution
// high level OS functions
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 3/9/17, valvano@mail.utexas.edu
#include "ST7735.h"
#include "os.h"
#include "ADC.h"
#include "UART2.h"
#include "efile.h"
#include "eDisk.h"
#include <string.h> 
#include <stdio.h>

#define NAME_FILE 6 // From eFile.c



char option;

void cmdLine_OptionE (void){
	int task, dirIdx, bytes,i;
	char data;
	char name[NAME_FILE];
	OutCRLF(); OutCRLF(); 
	
	UART_OutString ("Choose a task:"); OutCRLF();
	UART_OutString ("1.) Format the SD card"); OutCRLF();
	UART_OutString ("2.) Display the directory");  OutCRLF();
	UART_OutString ("3.) Print out the contents of a file as ASCII characters"); OutCRLF(); 
	UART_OutString ("4.) Delete a file"); OutCRLF(); OutCRLF();
	task = UART_InUDec();
	while ((task < 1) || (task > 4)){
			OutCRLF(); OutCRLF();
			UART_OutString ("Invalid measurement. Try again!"); OutCRLF(); 
			UART_OutString ("Measurement #: ");
			task = UART_InUDec();
		}
	OutCRLF(); OutCRLF();
	switch (task){
		case 1: eFile_Format(); UART_OutString ("SD card formatting done."); break;
		case 2: UART_OutString ("Directory:"); OutCRLF(); eFile_Directory(&UART_OutChar); break;
		case 4: UART_OutString ("Choose a file to delete: "); 
						UART_InString(name, NAME_FILE);
						eFile_Delete(name);
						break;
		case 3: UART_OutString ("Print contents of: "); 
						UART_InString(name, NAME_FILE);
						eFile_ROpen(name);
						dirIdx = get_DirIdx (name);
						bytes = get_BytesWritten(dirIdx);
						for (i = 0; i < bytes; i++){
							eFile_ReadNext (&data);
							UART_OutChar(data);
						}
						break;
						
	}
	OutCRLF();
}

void cmdLine_OptionD(int DataLost, int PIDWork, int FilterWork){
	int measurement;
	OutCRLF(); OutCRLF(); 
	
	UART_OutString ("Choose a measurement:"); OutCRLF();
	UART_OutString ("1.) Data lost "); OutCRLF();
	UART_OutString ("2.) Number of PID calculations finished");  OutCRLF();
	UART_OutString ("3.) Number of digital filter calculations finished"); OutCRLF(); OutCRLF();
	UART_OutString ("Measurement #: ");
	measurement = UART_InUDec();
	while ((measurement < 1) || (measurement > 3)){
			OutCRLF(); OutCRLF();
			UART_OutString ("Invalid measurement. Try again!"); OutCRLF(); 
			UART_OutString ("Measurement #: ");
			measurement = UART_InUDec();
		}
	OutCRLF(); OutCRLF();
	switch (measurement){
		case 1: UART_OutString ("Data lost = "); UART_OutUDec(DataLost); break;
		case 2: UART_OutString ("PID calculations finished = ");UART_OutUDec (PIDWork); break;
		case 3: UART_OutString ("Digital filter calculations finished = ");UART_OutUDec (FilterWork); break;
	}
	OutCRLF();
}

/// This functions allows the user to request either 1 or multiple samples from the ADC
void cmdLine_Start(int DataLost, int PIDWork, int FilterWork) {
  OutCRLF();
	// Lab 1
	UART_OutString ("Here are your options"); OutCRLF(); 
	UART_OutString ("a.) Get a single ADC sample."); OutCRLF(); 
	UART_OutString ("b.) Collect multiple ADC samples."); OutCRLF(); 
	UART_OutString ("c.) Write to LCD."); OutCRLF();
	// Lab 2	
	UART_OutString ("d.) Print performance measures."); OutCRLF(); 
	UART_OutString ("e.) Manage SD card."); OutCRLF(); OutCRLF();
	UART_OutString ("Option: ");
	option = UART_InChar ();
	UART_OutChar (option);
	while ((option != 'a') && (option != 'b') && (option != 'c') && (option != 'd') && (option != 'e')){
		OutCRLF(); OutCRLF(); 
		UART_OutString ("Invalid option. Try again!"); OutCRLF();  
		UART_OutString ("Option: ");
		option = UART_InChar ();
	}
	switch (option) {
		//case 'a': cmdLine_OptionA(); break;
		//case 'b': cmdLine_OptionB(); break;
		//case 'c': cmdLine_OptionC(); break;
		case 'd': cmdLine_OptionD(DataLost, PIDWork, FilterWork); break;
		case 'e': cmdLine_OptionE();
	}
	
}

void Interpreter(void) {
		while(1){
	//	cmdLine_Start (DataLost, PIDWork, FilterWork);
			cmdLine_Start (0,0,0);
		}
}
