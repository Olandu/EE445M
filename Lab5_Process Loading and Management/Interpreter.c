// *************Interpreter.c**************
// EE445M/EE380L.6 Lab 4 solution
// high level OS functions
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 3/9/17, valvano@mail.utexas.edu
#include <stdio.h>
#include "string.h" 
#include "ST7735.h"
#include "os.h"
#include "UART2.h"
#include "ff.h"
#include "loader.h"
#include "heap.h"

#define MAX_IN 20
#define NUM_CMD 5
char input[MAX_IN],token;
int option;

enum commands{PrintFile, Malloc,Free, ELF, Help};

char *usercmds[NUM_CMD] = {"PrintFile", "Malloc","Free", "ELF", "Help"};

void printFile(TCHAR *path){
	FIL File_Handle;
	FRESULT status;
	char data;
	UINT bytesRead;

	f_open(&File_Handle, path, FA_READ);
	do{
		status = f_read(&File_Handle, &data, 1, &bytesRead);
		if(bytesRead){
			UART_OutChar(data);
		}
	}while(status == FR_OK && bytesRead);
	f_close(&File_Handle);
}


int getOption(void){
	int option = -1, idx;
	 for(idx = 0; idx < NUM_CMD; idx++){
		 if(strcmp(input,usercmds[idx]) == 0)
			 return idx;
	 }
	return option;
}

void parseInput(char *token){
	int i = 0;
	while(*token != 0){
		input[i] = *token;
		token ++;
		i++;
	}
}

void cmdLine_Start(void) {
	OutCRLF();
  UART_OutString(">");
	UART_InString(input, MAX_IN);
	OutCRLF();
	option = getOption();
	switch(option){
		case PrintFile:
			UART_OutString(">Enter File Name: ");
			UART_InString(input, MAX_IN);
			OutCRLF();
			printFile(input);
			break;
		case Malloc:
			break;
		case Free:
			break;
		case ELF:
			break;
		case Help:
			break;
		default:
			UART_OutString("command not found");
	}
}

void Interpreter(void) {
		while(1){
			cmdLine_Start();
		}
}
