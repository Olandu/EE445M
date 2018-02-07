// cmdLine (Command Line)
// Lab 01
// Tejasree Ramanuja and Chioma Okorie
// January 27, 2018

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "ADC.h"
#include "OS.h"
#include "UART.h"

#define SAMPLES 1000
int32_t buffer[SAMPLES];
char option;
int flag = 0, lines = 5;

/// <summary>
/// This functions executes when the user chooses to request one sample from the ADC
/// </summary>
void cmdLine_OptionA (void) {
	int32_t channelNum = -1;
	
	OutCRLF(); OutCRLF(); 
	UART_OutString ("ADC Channel Number(0-11): ");
	channelNum = UART_InUDec();
	while ((channelNum < 0) || (channelNum > 11)){
		OutCRLF(); 
		UART_OutString ("Invalid channel number. Try again!"); OutCRLF(); 
		UART_OutString ("ADC Channel Number(0-11): ");
		channelNum = UART_InUDec();
	}
	OutCRLF(); OutCRLF(); 
	ADC_Open (channelNum);
	UART_OutString ("ADC Sample: ");
	int16_t result = ADC_In();
	UART_OutUDec (result);
	OutCRLF(); OutCRLF();
}




/// <summary>
/// This functions executes when the user chooses to request multiple samples from the ADC
/// Also allows the user to issue command to the LCD
/// User will specify channel number to sample, the sampling frequency, and LCD device #1 to view the output
/// </summary>

void cmdLine_OptionB (void){
	int32_t channelNum = -1, noSamples = -1;
	uint32_t fs = 0;
	
	OutCRLF(); OutCRLF(); 
	UART_OutString ("ADC Channel Number(0-11): ");
	channelNum = UART_InUDec();
	while ((channelNum < 0) || (channelNum > 11)){
		OutCRLF(); OutCRLF(); 
		UART_OutString ("Invalid channel number. Try again!"); OutCRLF(); 
		UART_OutString ("ADC Channel Number(0-11): ");
		channelNum = UART_InUDec();
	}
	OutCRLF(); OutCRLF(); 
	UART_OutString ("ADC Sampling frequency (Hz): ");
	fs = UART_InUDec();
	UART_OutString ("  Hz");
	OutCRLF(); OutCRLF();
	UART_OutString ("Number of samples (Max = 1000): ");
	noSamples = UART_InUDec();
	while ((noSamples < 1) || (noSamples > SAMPLES)){
		OutCRLF(); OutCRLF(); 
		UART_OutString ("Invalid sample size. Try again!"); OutCRLF(); 
		UART_OutString ("Number of Smaples: ");
		noSamples = UART_InUDec();
	}
	OutCRLF();
	ADC_Collect (channelNum, fs, buffer, noSamples);
	OutCRLF(); 
	for (int i = 0; i < noSamples; i++){
		UART_OutUDec (buffer[i]); OutCRLF();
	}
	OutCRLF(); OutCRLF(); 
}

void cmdLine_OptionC(void) {
	uint16_t device = 0, lineNum = 0, value = 0;
	char string[30];
	OutCRLF(); OutCRLF(); 
	
	
		UART_OutString ("Choose a device number:"); OutCRLF();
		UART_OutString ("1.) Device 1"); OutCRLF();
		UART_OutString ("2.) Device 2"); OutCRLF(); OutCRLF();
		UART_OutString ("Device #: ");
		device = UART_InUDec ();
		while ((device < 1) || (device > 2)){
			OutCRLF(); OutCRLF();
			UART_OutString ("Invalid device  number. Try again!"); OutCRLF(); 
			UART_OutString ("Device #: ");
			device = UART_InUDec ();
		}
		OutCRLF();
		UART_OutString ("Choose line number(1-5):"); OutCRLF();
		lineNum = UART_InUDec ();
		while ((lineNum < 1) || (lineNum > 5)){
			OutCRLF(); OutCRLF();
			UART_OutString ("Invalid line number. Try again!"); OutCRLF(); 
			UART_OutString ("Line #: ");
			lineNum = UART_InUDec ();
		}
		OutCRLF();
		UART_OutString ("Enter string: "); OutCRLF();
		UART_InString(string, 30);
		OutCRLF();
		UART_OutString ("Enter value: "); OutCRLF();
		value = UART_InUDec();
		ST7735_Message (device, lineNum - 1, string, value);	
}

/// This functions allows the user to request either 1 or multiple samples from the ADC
void cmdLine_Start (void) {
  OutCRLF();
	UART_OutString ("Here are your options"); OutCRLF(); 
	UART_OutString ("a.) Get a single ADC sample."); OutCRLF(); 
	UART_OutString ("b.) Collect multiple ADC samples."); OutCRLF(); 
	UART_OutString ("c.) Write to LCD."); OutCRLF(); OutCRLF();
	UART_OutString ("Option: ");
	option = UART_InChar ();
	UART_OutChar (option);
	while ((option != 'a') && (option != 'b') && (option != 'c')){
		OutCRLF(); OutCRLF(); 
		UART_OutString ("Invalid option. Try again!"); OutCRLF();  
		UART_OutString ("Option: ");
		option = UART_InChar ();
	}
	switch (option) {
		case 'a': cmdLine_OptionA(); break;
		case 'b': cmdLine_OptionB(); break;
		case 'c': cmdLine_OptionC(); break;
	}
}

