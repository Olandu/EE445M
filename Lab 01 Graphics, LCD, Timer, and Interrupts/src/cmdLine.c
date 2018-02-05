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

char option;

int flag = 0, lines = 5;

/// <summary>
/// This functions executes when the user chooses to request one sample from the ADC
/// </summary>
void cmdLine_OptionA (void) {
	int32_t channelNum = -1;
	
	OutCRLF(); OutCRLF(); 
	UART_OutString ("Alright! You have chosen to get one ADC sample."); OutCRLF(); 
	UART_OutString ("Please specify which ADC channel you would like to use."); OutCRLF(); 
	UART_OutString ("Choose any channel number from 0 to 11."); OutCRLF();
	UART_OutString ("ADC Channel Number: ");
	channelNum = UART_InUDec();
	while ((channelNum < 0) || (channelNum > 11)){
		OutCRLF(); 
		UART_OutString ("Oops! Sorry the specified channel does not exist. Try again!"); OutCRLF(); 
		UART_OutString ("Choose any channel number from 0 to 11."); OutCRLF(); OutCRLF();  
		UART_OutString ("ADC Channel Number: ");
		channelNum = UART_InUDec();
	}
	OutCRLF(); OutCRLF(); 
	ADC_Open (channelNum);
	UART_OutString ("Processing your request..."); OutCRLF();
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

#define SAMPLES 1000
int32_t buffer[SAMPLES];
void cmdLine_OptionB (void){
	int32_t channelNum = -1, noSamples = -1, device = 0;
	uint32_t fs = 0;
	char option;
	
	OutCRLF(); OutCRLF(); 
	UART_OutString ("Alright! You have chosen to collect multiple ADC samples."); OutCRLF(); OutCRLF();  
	UART_OutString ("Specify which ADC channel you would like to use."); OutCRLF(); 
	UART_OutString ("Choose any channel number from 0 to 11."); OutCRLF();
	UART_OutString ("ADC Channel Number: ");
	channelNum = UART_InUDec();
	while ((channelNum < 0) || (channelNum > 11)){
		OutCRLF(); OutCRLF(); 
		UART_OutString ("Oops! Sorry the specified channel does not exist. Try again!"); OutCRLF(); 
		UART_OutString ("Choose any channel number from 0 to 11."); OutCRLF(); OutCRLF();  
		UART_OutString ("ADC Channel Number: ");
		channelNum = UART_InUDec();
	}
	OutCRLF(); OutCRLF(); 
	UART_OutString ("Specify the ADC sampling frequency (Hz)"); OutCRLF();
	UART_OutString ("ADC Sampling frequency: ");
	fs = UART_InUDec();
	UART_OutString ("  Hz");
	OutCRLF(); OutCRLF();
	UART_OutString ("Specify the number of samples you desire to collect."); OutCRLF();
	UART_OutString ("Note: Maximum number of samples = 1000"); OutCRLF();
	UART_OutString ("Number of samples: ");
	noSamples = UART_InUDec();
	while ((noSamples <= 0) || (noSamples >= SAMPLES)){
		OutCRLF(); OutCRLF(); 
		UART_OutString ("Oops! Invalid number of . Try again!"); OutCRLF(); 
		UART_OutString ("Choose any channel number from 0 to 11."); OutCRLF(); OutCRLF();  
		UART_OutString ("Number of Smaples: ");
		noSamples = UART_InUDec();
	}
	OutCRLF(); OutCRLF(); 
	UART_OutString ("You chose Channel # ");
	UART_OutUDec (channelNum);
	UART_OutString (", ");
	UART_OutUDec (fs);
	UART_OutString (" Hz, and ");
	UART_OutUDec (noSamples);
	UART_OutString (" samples.");
	ADC_Collect (channelNum, fs, buffer, noSamples);
	OutCRLF(); 
	UART_OutString ("Processing your request..."); OutCRLF();
	UART_OutString ("Here are your samples: "); OutCRLF();
	for (int i = 0; i < noSamples; i++){
		UART_OutUDec (buffer[i]); OutCRLF();
	}
	OutCRLF(); OutCRLF(); 
//	UART_OutString ("What would you like to do with the samples?"); OutCRLF();
//	UART_OutString ("a.) Output the samples to one of the LCD Displays"); OutCRLF();
//    UART_OutString ("b.) Umm, nothing!"); OutCRLF(); OutCRLF();
//	UART_OutString ("Choose either option a or b."); OutCRLF();
//	UART_OutString ("Option: "); 
//	option = UART_InChar();
//	while ((option != 'a') && (option != 'b')){
//		OutCRLF(); OutCRLF(); 
//		UART_OutString ("Oops! You chose an invalid option. Try again!"); OutCRLF(); 
//		UART_OutString ("Choose either option a or b to begin."); OutCRLF(); OutCRLF();  
//		UART_OutString ("Option: ");
//		option = UART_InChar ();
//	}
//	if (option == 'a'){
//		OutCRLF(); OutCRLF(); 
//		UART_OutString ("Available LCD devices:"); OutCRLF();
//		UART_OutString ("1.) Device 1"); OutCRLF();
//		UART_OutString ("2.) Device 2"); OutCRLF(); OutCRLF();
//		UART_OutString ("Choose either device \"1\" or \"2.\""); OutCRLF();
//		UART_OutString ("Device #: ");
//		device = UART_InUDec ();
//		while ((device != 1) && (device != 2)){
//			OutCRLF(); OutCRLF();
//			UART_OutString ("Oops! You chose an invalid device. Try again!"); OutCRLF(); 
//			UART_OutString ("Choose either device \"1\" or \"2.\""); OutCRLF(); OutCRLF();  
//			UART_OutString ("Option: ");
//			UART_OutString ("Device #: ");
//			device = UART_InUDec ();
//		}
//		for (int i = 0; i < noSamples; i++){
//			ST7735_Message (device, i, "ADC Value", buffer[i], 0);
//		}
//		if (noSamples != lines){
//			for (int i = noSamples; i < lines + 1; i++){
//				ST7735_Message (device, i, "--------------------", buffer[i], 1);
//			}
//		}
//		device = 0;
//	}
}

void cmdLine_OptionC(void) {
	uint16_t device = 0, lineNum = 0, value = 0;
	char string[30];
	OutCRLF(); OutCRLF(); 
	
	
		UART_OutString ("Available LCD devices:"); OutCRLF();
		UART_OutString ("1.) Device 1"); OutCRLF();
		UART_OutString ("2.) Device 2"); OutCRLF(); OutCRLF();
		UART_OutString ("Choose either device \"1\" or \"2.\""); OutCRLF();
		UART_OutString ("Device #: ");
		device = UART_InUDec ();
		while ((device != 1) && (device != 2)){
			OutCRLF(); OutCRLF();
			UART_OutString ("Oops! You chose an invalid device. Try again!"); OutCRLF(); 
			UART_OutString ("Choose either device \"1\" or \"2.\""); OutCRLF(); OutCRLF();  
			UART_OutString ("Option: ");
			UART_OutString ("Device #: ");
			device = UART_InUDec ();
		}
		OutCRLF();
		UART_OutString ("Choose which line (1-5) to output to:"); OutCRLF();
		lineNum = UART_InUDec ();
		while ((lineNum < 1) || (lineNum > 5)){
			OutCRLF(); OutCRLF();
			UART_OutString ("Oops! You chose an invalid device. Try again!"); OutCRLF(); 
			UART_OutString ("Choose an appropriate line (1-5)."); OutCRLF(); OutCRLF();  
			UART_OutString ("Option: ");
			UART_OutString ("Line #: ");
			lineNum = UART_InUDec ();
		}
		OutCRLF();
		UART_OutString ("Enter string: "); OutCRLF();
		UART_InString(string, 30);
		OutCRLF();
		UART_OutString ("Enter value: "); OutCRLF();
		value = UART_InUDec();
		
		ST7735_Message (device, lineNum - 1, string, value, 0);
		
}

/// This functions allows the user to request either 1 or multiple samples from the ADC
void cmdLine_Start (void) {
	if (!flag){
  OutCRLF();
	// Make a new function out of this
  UART_OutChar('-');
  UART_OutChar('-');
  UART_OutChar('>');
	UART_OutString ("Hello! Welcome to EE 445M. Let's get started!"); OutCRLF(); OutCRLF();
	} 
	UART_OutString ("Things you can do:"); OutCRLF(); 
	UART_OutString ("a.) Get a single ADC sample."); OutCRLF(); 
	UART_OutString ("b.) Collect multiple ADC samples & display the values to either device 1 or 2."); OutCRLF(); 
	UART_OutString ("c.) Write to LCD."); OutCRLF(); OutCRLF();
	UART_OutString ("Option: ");
	option = UART_InChar ();
	UART_OutChar (option);
	while ((option != 'a') && (option != 'b') && (option != 'c')){
		OutCRLF(); OutCRLF(); 
		UART_OutString ("Oops! You chose an invalid option. Try again!"); OutCRLF(); 
		UART_OutString ("Choose either option a or b to begin."); OutCRLF(); OutCRLF();  
		UART_OutString ("Option: ");
		option = UART_InChar ();
	}
	switch (option) {
		case 'a': cmdLine_OptionA(); break;
		case 'b': cmdLine_OptionB(); break;
		case 'c': cmdLine_OptionC(); break;
	}
	
	OutCRLF(); OutCRLF(); 
	UART_OutString ("Your request has been successfully serviced. Bye!"); OutCRLF(); OutCRLF();  flag = 1;
}

