// *************Interpreter.c**************
// EE445M/EE380L.6 Lab 4 solution
// high level OS functions
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 3/9/17, valvano@mail.utexas.edu
#include "ST7735.h"
#include "os.h"
#include "UART2.h"
#include <string.h> 
#include <stdio.h>

#define NAME_FILE 6 // From eFile.c

char option;


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
		//case 'd': cmdLine_OptionD(DataLost, PIDWork, FilterWork); break;
		//case 'e': cmdLine_OptionE();
	}
	
}

void Interpreter(void) {
		while(1){
	//	cmdLine_Start (DataLost, PIDWork, FilterWork);
			cmdLine_Start (0,0,0);
		}
}
