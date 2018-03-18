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


void Interpreter(void) {
		while(1){
//		cmdLine_Start (DataLost, PIDWork, FilterWork);
		}
}

char option;

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
	UART_OutString ("d.) Print performance measures."); OutCRLF(); OutCRLF(); 
	UART_OutString ("Option: ");
	option = UART_InChar ();
	UART_OutChar (option);
	while ((option != 'a') && (option != 'b') && (option != 'c') && (option != 'd')){
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
	}
	
}
