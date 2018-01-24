// os.c
// Runs on TM4C123
// Chioma Okorie and Tejasree Ramanuja
// January 23, 2018

#include <stdint.h>
#include "OS.h"


//----------OS_AddPeriodicThread------------
// Initalizes a periodic task
// Inputs:  task: the periodic task to be performed
//				 period: determines how often the task execute (milliseconds)
//				 priority: determines the priority of task
// Output: 1 if successful, 0 if this thread can not be added
int OS_AddPeriodicThread(void(*task)(void), uint32_t period, uint32_t priority){
	return 1;
}


//---------- OS_ClearPeriodicTime------------
// Resets the 32-bit global timer
// Input:  None
// Output: None
void OS_ClearPeriodicTime(void){
	
}

//---------- OS_ReadPeriodicTime------------
// Returns the current value of the global counter
// Input:  None 
// Output: 32-bit global counter
uint32_t OS_ReadPeriodicTime(void){
	return 0;
}

