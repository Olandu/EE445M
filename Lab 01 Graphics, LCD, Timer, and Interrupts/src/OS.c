// os.c
// Runs on TM4C123
// Tejasree Ramanuja and Chioma Okorie
// January 23, 2018

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "OS.h"

#define PF1                     (*((volatile uint32_t *)0x40025008))
	
int32_t OS_Counter = 0;


// background task 
void (*PeriodicTask)(void);   // user function


///----------OS_AddPeriodicThread------------
/// <summary>
/// Initializes a periodic task
///  </summary>
/// <param name = "task"> periodic task to be performed </param>
///	<param name = "period"> determines how often the task execute (milliseconds) </param>
///	<param name = "priority"> determines the priority of task </param>
///
/// </returns> Return 1 if successful, 0 if this thread can not be added
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

#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value
// Initialize SysTick with busy wait running at bus clock.
volatile uint32_t Time1, elapsedTime;
void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;                   // disable SysTick during setup
  NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;  // maximum reload value
  NVIC_ST_CURRENT_R = 0;                // any write to current clears it
                                        // enable SysTick with core clock
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC;
}

void Timer4A_Handler(void){
  TIMER4_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER4A timeout
	//Time1 = NVIC_ST_CURRENT_R&0x00FFFFFF;
	//PF1 ^= 0x02;
	//PF1 ^= 0x02;
	OS_Counter ++;
  (*PeriodicTask)();                // execute user task
	//PF1 ^= 0x02;
	//elapsedTime = (Time1 - NVIC_ST_CURRENT_R)&0x00FFFFFF;
}

///---------- OS_ClearPeriodicTime------------
/// <summary>
/// Resets the 32-bit global timer
/// </summary>
// Input:  None
// Output: None
void OS_ClearPeriodicTime(void){
	OS_Counter = 0;           
}

///---------- OS_ReadPeriodicTime------------
/// <summary>
/// Returns the current value of the global counter
/// </summary>
/// <returns> 32-bit global counter
uint32_t OS_ReadPeriodicTime(void){
	return OS_Counter;
}

