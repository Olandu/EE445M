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

char option;

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

void cmdLine_OptionB (void){
	int32_t channelNum = -1, noSamples = -1;
	uint32_t fs = 0;
	
	OutCRLF(); OutCRLF(); 
	UART_OutString ("Alright! You have chosen to collect multiple ADC samples."); OutCRLF(); 
	UART_OutString ("Specify which ADC channel you would like to use."); OutCRLF(); 
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
	OutCRLF();
	UART_OutString ("Specify the ADC sampling frequency (Hz)"); OutCRLF();
	UART_OutString ("ADC Sampling frequency: ");
	fs = UART_InUDec();
	UART_OutString ("  Hz");
	OutCRLF();
	UART_OutString ("Specify the number of samples you desire to collect."); OutCRLF();
	UART_OutString ("Note: Maximum number of samples = 5");
	noSamples = UART_InUDec();
	while ((noSamples < 0) || (noSamples > 5)){
		OutCRLF(); 
		UART_OutString ("Oops! Invalid number of . Try again!"); OutCRLF(); 
		UART_OutString ("Choose any channel number from 0 to 11."); OutCRLF(); OutCRLF();  
		UART_OutString ("Number of Smaples: ");
		noSamples = UART_InUDec();
	}
	uint32_t buffer[noSamples];
	ADC_Collect (channelNum, fs, buffer, noSamples);
	OutCRLF(); 
	UART_OutString ("Processing your request..."); OutCRLF();
	UART_OutString ("Here are your samples: "); OutCRLF();
	for (int i = 0; i < noSamples; i++){
		UART_OutUDec (buffer[i]); OutCRLF();
	}
	
}

void cmdLine_Start (void) {
  uint32_t n;
	
  OutCRLF();
	// Make a new function out of this
  UART_OutChar('-');
  UART_OutChar('-');
  UART_OutChar('>');
	UART_OutString ("Hello! Welcome to EE 445M. Let's get started!"); OutCRLF(); OutCRLF(); 
	UART_OutString ("Things you can do:"); OutCRLF(); 
	UART_OutString ("a.) Get a single ADC sample."); OutCRLF(); 
	UART_OutString ("b.) Collect multiple ADC samples & display the values to either device 1 or 2."); OutCRLF(); OutCRLF(); 
	UART_OutString ("Choose either option a or b to begin."); OutCRLF(); 
	UART_OutString ("Option: ");
	option = UART_InChar ();
	while ((option != 'a') || (option != 'b')){
		OutCRLF(); 
		UART_OutString ("Oops! You chose an invalid option. Try again!"); OutCRLF(); 
		UART_OutString ("Choose either option a or b to begin."); OutCRLF(); OutCRLF();  
		UART_OutString ("Option: ");
		option = UART_InChar ();
	}
	switch (option) {
		case 'a': cmdLine_OptionA(); break;
		case 'b': cmdLine_OptionB(); break;
	}
	
	OutCRLF(); OutCRLF(); 
	UART_OutString ("What would you like to do now?");
}




