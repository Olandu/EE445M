// os.c
// Runs on TM4C123
// Chioma Okorie and Tejasree Ramanuja
// January 23, 2018

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "OS.h"

int32_t OS_Counter = 0;

//----------OS_AddPeriodicThread------------
// Initalizes a periodic task
// Inputs:  task: the periodic task to be performed
//				 period: determines how often the task execute (milliseconds)
//				 priority: determines the priority of task
// Output: 1 if successful, 0 if this thread can not be added

// background task 
void (*PeriodicTask)(void);   // user function
int OS_AddPeriodicThread(void(*task)(void), uint32_t period, uint32_t priority){
	SYSCTL_RCGCTIMER_R |= 0x10;   // 0) activate TIMER4
  PeriodicTask = task;          // user function
  TIMER4_CTL_R = 0x00000000;    // 1) disable TIMER4A during setup
  TIMER4_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER4_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER4_TAILR_R = period-1;    // 4) reload value
  TIMER4_TAPR_R = 0;            // 5) bus clock resolution
  TIMER4_ICR_R = 0x00000001;    // 6) clear TIMER4A timeout flag
  TIMER4_IMR_R = 0x00000001;    // 7) arm timeout interrupt
	priority = (priority & 0x07) << 21; // mask priority (nvic bits 23-21)
  NVIC_PRI17_R = (NVIC_PRI17_R&0xF00FFFFF);
	NVIC_PRI17_R = (NVIC_PRI17_R | priority); // 8) priority
// interrupts enabled in the main program after all devices initialized
// vector number 51, interrupt number 35
  NVIC_EN2_R = 1<<(70-64);      // 9) enable IRQ 70 in NVIC 
  TIMER4_CTL_R = 0x00000001;    // 10) enable TIMER4A
	return 1;
}

void Timer4A_Handler(void){
  TIMER4_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER4A timeout
	OS_Counter ++;
  (*PeriodicTask)();                // execute user task
}

//---------- OS_ClearPeriodicTime------------
// Resets the 32-bit global timer
// Input:  None
// Output: None
void OS_ClearPeriodicTime(void){
	OS_Counter = 0;           
}

//---------- OS_ReadPeriodicTime------------
// Returns the current value of the global counter
// Input:  None 
// Output: 32-bit global counter
uint32_t OS_ReadPeriodicTime(void){
	return OS_Counter;
}

