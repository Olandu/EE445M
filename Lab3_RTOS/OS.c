// filename **********OS.c***********
// Real Time Operating System for Labs 2 and 3 
// Jonathan W. Valvano 2/20/17, valvano@mail.utexas.edu
// EE445M/EE380L.12
// Modified by Chioma Okorie and Tejasree Ramanuja
/**
 * @file os.h
 * @brief Real Time Operating System for EE445M Lab2
 * @author Chioma Okorie
 * @author Tejasree Ramanuja
*/
#include <stdint.h>
#include "OS.h"
#include "PLL.h"
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "UART.h"


// edit these depending on your clock        
#define TIME_1MS    80000          
#define TIME_2MS    (2*TIME_1MS)  
#define TIME_500US  (TIME_1MS/2)  
#define TIME_250US  (TIME_1MS/5)  
#define PERIOD TIME_500US


#define NVIC_ST_CTRL_R          (*((volatile uint32_t *)0xE000E010))
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_R        (*((volatile uint32_t *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile uint32_t *)0xE000E018))
#define NVIC_INT_CTRL_R         (*((volatile uint32_t *)0xE000ED04))
#define NVIC_INT_CTRL_PENDSTSET 0x04000000  // Set pending SysTick interrupt
#define NVIC_SYS_PRI3_R         (*((volatile uint32_t *)0xE000ED20))  // Sys. Handlers 12 to 15 Priority

#define PF0                     (*((volatile uint32_t *)0x40025004))
#define PF4                     (*((volatile uint32_t *)0x40025040))
#define PF1                     (*((volatile uint32_t *)0x40025008))
#define PE1  (*((volatile unsigned long *)0x40024008))
	
#define SysTimeReload   0xFFFFFFFF
#define NUMTHREADS  10        // maximum number of threads
#define STACKSIZE   128       // number of 32-bit words in stack
#define PRILEVELS		8					// Maximum number of priority levels we can have is 7

#define Lab2 0
#define Lab3 1
#define PriScheduler 0

#if Lab3
long MaxJitter1;             // largest time jitter between interrupts in usec
long MaxJitter2;             // largest time jitter between interrupts in usec
#define JITTER_SIZE 64
unsigned long const Jitter_Size = JITTER_SIZE;
unsigned long JitterHistogram1[JITTER_SIZE]={0,};
unsigned long JitterHistogram2[JITTER_SIZE]={0,};
#endif

// function definitions in osasm.s
void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts
int32_t StartCritical(void);
void EndCritical(int32_t primask);
void StartOS(void);




//function prototypes in os.c
void Timer3_MS(unsigned long period);
void Timer0A_Init(void);


unsigned long SystemTime_Ms;
unsigned long NumThreads = 0;


struct tcb{
  int32_t *sp;                // pointer to stack (valid for threads not running
  struct tcb *next;           // linked-list pointer
	struct tcb *prev;
	int Id;
	int sleep;
	int status;
	uint8_t priority;						// Range: 0 to 5 because 6 adn 7 being used for Systick and PendSV respectively
	Sema4Type *blocked;
	struct tcb *nextBlocked;
	struct tcb *Pri_Next;
	
};

typedef struct tcb tcbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;
tcbType *NextRunPt;
tcbType *LinkPt;
int32_t Stacks[NUMTHREADS][STACKSIZE];

// Priority Scheduling Data Structures
tcbType *PriPt[PRILEVELS] = {0};						// Each correspongding slot will lead to the linked list associated with the priority (index)
tcbType *PriLastPt[PRILEVELS] = {0};
int32_t Pri_Total[PRILEVELS] = {0};					// 0 if none else >0; Modify when any of the threads kill themselves
int32_t Pri_Available[PRILEVELS] = {0};			// Modified when any of the threads are sleeping or blocked...if 0, scheduler moves to the next priority level
void AddPriThread (tcbType *thread);


void SetInitialStack(int i){
  tcbs[i].sp = &Stacks[i][STACKSIZE-16]; // thread stack pointer
  Stacks[i][STACKSIZE-1] = 0x01000000;   // thumb bit
  Stacks[i][STACKSIZE-3] = 0x14141414;   // R14
  Stacks[i][STACKSIZE-4] = 0x12121212;   // R12
  Stacks[i][STACKSIZE-5] = 0x03030303;   // R3
  Stacks[i][STACKSIZE-6] = 0x02020202;   // R2
  Stacks[i][STACKSIZE-7] = 0x01010101;   // R1
  Stacks[i][STACKSIZE-8] = 0x00000000;   // R0
  Stacks[i][STACKSIZE-9] = 0x11111111;   // R11
  Stacks[i][STACKSIZE-10] = 0x10101010;  // R10
  Stacks[i][STACKSIZE-11] = 0x09090909;  // R9
  Stacks[i][STACKSIZE-12] = 0x08080808;  // R8
  Stacks[i][STACKSIZE-13] = 0x07070707;  // R7
  Stacks[i][STACKSIZE-14] = 0x06060606;  // R6
  Stacks[i][STACKSIZE-15] = 0x05050505;  // R5
  Stacks[i][STACKSIZE-16] = 0x04040404;  // R4
}

void OtherInits(void){
	 LCD_Init();
	 UART_Init();	
}

void UnchainTCB(void){
	RunPt->prev->next = RunPt->next;
	RunPt->next->prev = RunPt->prev;
//	LinkPt = RunPt->next;
	NumThreads--;
}

void ChainTCB(tcbType *newActiveThread){
	tcbType *last = RunPt->prev;
//	tcbType *last = LinkPt->prev;
	Pri_Available[newActiveThread->priority] = Pri_Available[newActiveThread->priority] + 1;
	newActiveThread->next = RunPt;
//	newActiveThread->next = LinkPt;
	RunPt->prev = newActiveThread;
//	LinkPt->prev = newActiveThread;
	newActiveThread->prev = last;
	last->next = newActiveThread;
	NumThreads++;
}

void AddBlockedTCB_Sema4(Sema4Type *semaPt){
	//find the end of the blocked list
	if(semaPt->blockedThreads == 0){//add head
		semaPt->blockedThreads = RunPt;
		semaPt->blockedThreads->nextBlocked = 0; //current thread is the end of the blocked list
	}
	else{//find the end of the list for the tcbs blocked on the sema4
		tcbType *tail = semaPt->blockedThreads;
		while(tail->nextBlocked != 0){
			tail = tail->nextBlocked;
		}
		tail->nextBlocked = RunPt;
		tail->nextBlocked->nextBlocked = 0;
	}
}

tcbType* RemoveBlockedTCB_sema4(Sema4Type *semaPt){// assumes that at least one thread is blocked on this sema4
	tcbType *head = semaPt->blockedThreads;
	semaPt->blockedThreads = head->nextBlocked;
	return head;
}

void Block(Sema4Type *semaPt){
	RunPt->blocked = semaPt; //current thread point to the semap4 it is blocked on
	Pri_Available[RunPt->priority] = Pri_Available[RunPt->priority] - 1;
	UnchainTCB(); // remeove current tcb from active list (does not free tcb)
	AddBlockedTCB_Sema4(semaPt); // add tcb to the linked list of blocked threads
	OS_Suspend();
}

void UnBlock(Sema4Type *semaPt){
		//remove the first tcb blocked on this sema4 from the blocked list
		tcbType *blockedTCB = RemoveBlockedTCB_sema4(semaPt);
		//add tcb to the active list
		ChainTCB(blockedTCB); 
		blockedTCB->blocked = 0; //wake up threads
}

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
// input:  none
// output: none
/** @brief  OS_Initialization
 *	initialize operating system, disable interrupts until OS_Launch
 *	initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
 *  @param  none
 *  @return none
*/
void OS_Init(void){
	OS_DisableInterrupts();
  PLL_Init(Bus80MHz);         // set processor clock to 50 MHz
	OtherInits();
	SystemTime_Ms = 0;
	for(int i = 0; i < NUMTHREADS; i++){ // initialize the state of the tcb (-1 means tcbs are free)
		tcbs[i].status = -1;
	}
	for(int i = 0; i < PRILEVELS; i++){
		PriPt[i] = 0;
	}
	Timer3_MS(TIME_1MS);
	Timer0A_Init();
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xD0000000; // priority 6
	NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0xFF00FFFF)|0x00E00000; // priority 7
	RunPt = &tcbs[0]; 
}

//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
/** @brief  OS_Launch
 *	start the scheduler, enable interrupts
 *	TimeSlice field is ignored in lab2 
 *  @param  number of 12.5ns clock cycles for each time slice
 *  @return none
*/
void OS_Launch(unsigned long theTimeSlice){
	NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
  NVIC_ST_CTRL_R = 0x00000007; // enable, core clock and interrupt arm
	StartOS();
}

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
//Note: This function no longer triggers PendSV directly
//This is done because we want a sleeping task to be able to suspend itself (Systick has higher priority)
/** @brief  OS_Suspend
 *	suspend execution of currently running thread
 *	scheduler will choose another thread to execute
 *  Can be used to implement cooperative multitasking 
 *  Same function as OS_Sleep(0)
 *  @param  none
 *  @return none
*/
void OS_Suspend(void){
	 NVIC_ST_CURRENT_R = 0;      // any write to current clears it
	 //NVIC_INT_CTRL_R = 0x10000000;    // trigger PendSV
	 NVIC_INT_CTRL_R = 0x04000000; // trigger SysTick
}



// ******** SysTick_Handler ************
/** @brief  SysTick_Handler
 *	Determines which task to run next
 *	Performs context switch
 *  Lab2 uses a preemptive round robin scheduler
 *  @param  none
 *  @return none
*/
void SysTick_Handler(void){
	//don't use RunPt here because we want to first save the current sp before switching
	// this is done in PendSV_handler (OSasm.s)
	#if !PriScheduler 
	NextRunPt = RunPt->next;
	while(NextRunPt->sleep || NextRunPt->blocked){	// Accounts for both sleeping and blocked threads
		NextRunPt = NextRunPt->next;
	}
	#else
	PE1 ^= 0x02;
	PE1 ^= 0x02;
	int8_t priority = -1;
	for (priority = 0; priority < PRILEVELS; priority++){
		if (Pri_Available[priority] >= 1){break;}
	}
	if (priority == RunPt->priority){	
		NextRunPt = RunPt->Pri_Next;
		while(NextRunPt->sleep || NextRunPt->blocked){	// Accounts for both sleeping and blocked threads
			NextRunPt = NextRunPt->Pri_Next;
		}  
	} else {
		if (priority != PRILEVELS){
			NextRunPt = PriPt[priority];
			while(NextRunPt->sleep || NextRunPt->blocked){	// Accounts for both sleeping and blocked threads
				NextRunPt = NextRunPt->Pri_Next;
			}
		}
	}
	#endif
	NVIC_INT_CTRL_R = 0x10000000;    // trigger PendSV
	PE1 ^= 0x02;
}

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
/** @brief  OS_AddThread
 *	add a foregound thread to the scheduler
 *	stackSize and priority fields are ignored in this lab
 *  @param  pointer to a void/void foreground task
 *  @param  number of bytes allocated for its stack
 *  @param  priority, 0 is highest, 5 is the lowest
 *  @return 1 if successful, 0 if this thread can not be added
*/
int OS_AddThread(void(*task)(void),unsigned long stackSize, unsigned long priority){int32_t status;
	if(NumThreads >= NUMTHREADS){
		return 0; // thread can not be added
	}
	status = StartCritical();
	// find free tcb
	int freetcb;
	for( freetcb = 0; freetcb < NUMTHREADS; freetcb++){
		if(tcbs[freetcb].status == -1){
			break;
		}
	}
	if(NumThreads == 0){
		tcbs[NumThreads].next = &tcbs[0];
		tcbs[NumThreads].prev = &tcbs[0];
		RunPt = &tcbs[0];
		tcbs[0].priority = priority;
	}
	else{ //traverse linked list of tcb and find the last thread
		if(RunPt->status == -1){
			EndCritical(status);
			return 0;
		}
		//doubly-linked list
		tcbType *last = RunPt->prev; 
		tcbs[freetcb].next = RunPt;		
		RunPt->prev = &tcbs[freetcb];
		tcbs[freetcb].prev = last;
		last->next = &tcbs[freetcb];
		
	}
	SetInitialStack(freetcb);
	Stacks[freetcb][STACKSIZE-2] = (int32_t)(task); // PC
	//set inittial state of tcb fields
	tcbs[freetcb].sleep = 0;
	tcbs[freetcb].status = 0; // tcb is in use (not free)
	tcbs[freetcb].priority = priority;
	tcbs[freetcb].Id = freetcb;
	NumThreads++;
	
	#if PriScheduler
	AddPriThread (&tcbs[freetcb]);
	#endif
	EndCritical(status);
	return 1;     //successful
}

void AddPriThread (tcbType *thread){
	uint8_t priority = thread->priority;
	
	if(Pri_Total[priority] == 0){
		PriPt[priority] = thread; 
		PriLastPt[priority] = thread;
		thread->Pri_Next = thread;
	} else {	// Add it to the beginning of the priority "linked list"
		thread->Pri_Next = PriLastPt[priority]->Pri_Next;
		PriLastPt[priority]->Pri_Next = thread;
		PriPt[priority] = thread; 
	}
	
	Pri_Total[priority]++; Pri_Available[priority]++;
}

//******** OS_PeriodicThreadInit *************** 
void (*PeriodicThread1)(void); 
/** @brief  PeriodicThread1_init
 *	Initializes a periodic task
 *  @param  period given in system time units (12.5ns)
 *  @param  priority of the background periodic task
 *  @return none
*/
void Timer4_Init(void(*task)(void), uint32_t period, uint32_t priority){long sr;
	sr = StartCritical();
	SYSCTL_RCGCTIMER_R |= 0x10;   // 0) activate TIMER4
	PeriodicThread1 = task;          // user function
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
	EndCritical(sr);
}

void (*PeriodicThread2)(void);   // user function
// ***************** TIMER1_Init ****************
// Activate TIMER1 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
// Outputs: none
void Timer1_Init(void(*task)(void), uint32_t period,uint32_t priority){long sr;
	sr = StartCritical();
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  PeriodicThread2 = task;          // user function
  TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = period-1;    // 4) reload value
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00008000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 37, interrupt number 21
  NVIC_EN0_R = 1<<21;           // 9) enable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x00000001;    // 10) enable TIMER1A
	EndCritical(sr);
}
/** @brief  Timer4A_Handler
 *	Executes periodic background task
 *  @param  none
 *  @return none
*/
uint32_t PerTask1Counter = 0;
uint32_t PerTask1Period;
void Timer4A_Handler(void){
	unsigned long thisTime = OS_Time();
	unsigned static long LastTime;
	long jitter;
	
  TIMER4_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER4A timeout
  (*PeriodicThread1)();                // execute user task
	
#if Lab3	
	if (PerTask1Counter){
		unsigned long diff = OS_TimeDifference(LastTime,thisTime);
      if(diff>PerTask1Period){
        jitter = (diff-PerTask1Period+4)/8;  // in 0.1 usec
      }else{
        jitter = (PerTask1Period-diff+4)/8;  // in 0.1 usec
      }
      if(jitter > MaxJitter1){
        MaxJitter1 = jitter; // in usec
      }       // jitter should be 0
      if(jitter >= Jitter_Size){
        jitter = JITTER_SIZE-1;
      }
      JitterHistogram1[jitter]++; 
	}
	 LastTime = thisTime;
	 PerTask1Counter++;
#endif
}

uint32_t PerTask2Counter = 0;
uint32_t PerTask2Period;
void Timer1A_Handler(void){
	unsigned long thisTime = OS_Time();
	unsigned static long LastTime;
	long jitter;
	
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER1A timeout
  (*PeriodicThread2)();                // execute user task
	
#if Lab3
	if (PerTask2Counter){
		unsigned long diff = OS_TimeDifference(LastTime,thisTime);
      if(diff>PerTask2Period){
        jitter = (diff-PerTask2Period+4)/8;  // in 0.1 usec
      }else{
        jitter = (PerTask2Period-diff+4)/8;  // in 0.1 usec
      }
      if(jitter > MaxJitter2){
        MaxJitter2 = jitter; // in usec
      }       // jitter should be 0
      if(jitter >= Jitter_Size){
        jitter = JITTER_SIZE-1;
      }
      JitterHistogram2[jitter]++; 
	}
	 LastTime = thisTime;
	 PerTask2Counter++;
#endif
}

void Jitter (void) {
	#if Lab3
	ST7735_Message(0, 1, "Jitter 0.1us = ", MaxJitter1);	// Jitter for Periodic Task 1
	ST7735_Message(0, 2, "Jitter 0.1us = ", MaxJitter2);	// Jitter for Periodic Task 2
	#endif
}
//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
/** @brief  OS_AddPeriodicThread
 *	add a background periodic task and initializes the periodic timer
 *  typically this function receives the highest priority
 *  You are free to select the time resolution for this function
 *  It is assumed that the user task will run to completion and return
 *  This task can not spin, block, loop, sleep, or kill
 *  This task can call OS_Signal  OS_bSignal	 OS_AddThread
 *  This task does not have a Thread ID
 *  @param  pointer to a void/void background function
 *  @param  period given in system time units (12.5ns)
 *  @param  priority 0 is the highest, 5 is the lowest
 *  @return 1 if successful, 0 if this thread can not be added
*/
unsigned long NumPeriodicTasks = 0;
int OS_AddPeriodicThread(void(*task)(void),unsigned long period, unsigned long priority){ 
		if(NumPeriodicTasks == 0){
			PerTask1Period = period;
			Timer4_Init(task,period,priority);
		}
		else if(NumPeriodicTasks == 1){
		 PerTask2Period = period;
		 Timer1_Init(task,period,priority);
		}
		else{
			return 0; // cannot add more than two threads
		}
		NumPeriodicTasks++;
		return 1;
}


//******** OS_EventThreadInit *************** 
void (*SW1_EventThread) (void);
unsigned long SW1_Priority;
unsigned long LastPF4;
/** @brief  SW1_init
 *	Initializes the TM4C123 built-in SW1
 *  @param  priority of the background periodic task
 *  @return none
*/
void SW1_init (unsigned long priority){unsigned long volatile delay;
	SYSCTL_RCGCGPIO_R |= 0x00000020; 	// (a) activate clock for port F
  delay = SYSCTL_RCGCGPIO_R;
  GPIO_PORTF_CR_R = 0x10;           // allow changes to PF4
  GPIO_PORTF_DIR_R &= ~0x10;    		// (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;  		//     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;     		//     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000F0000; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       		//     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x10;     		//     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     		// (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;     		//     PF4 is not edges
	GPIO_PORTF_IEV_R &= ~0x10;     		//     PF4 falling edges

	GPIO_PORTF_ICR_R = 0x10;      		// (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      		// (f) arm interrupt on PF4
	SW1_Priority = priority;
	priority = (priority & 0x07) << 21;	// NVIC priority bit (21-23)
  //NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF); // clear priority
	NVIC_PRI7_R = (NVIC_PRI7_R | priority); 
  NVIC_EN0_R = 0x40000000;      		// (h) enable interrupt 30 in NVIC 
	LastPF4 = PF4;
}


void (*SW2_EventThread) (void);
unsigned long LastPF0;
unsigned long SW2_Priority;
void SW2_init (unsigned long priority){
	SYSCTL_RCGCGPIO_R |= 0x00000020; 	// (a) activate clock for port F
  while((SYSCTL_PRGPIO_R & 0x00000020) == 0){};
	GPIO_PORTF_LOCK_R = 0x4C4F434B; // unlock GPIO Port F
  GPIO_PORTF_DIR_R &= ~0x01;    		// (c) make PF0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x01;  		//     disable alt funct on PF0
  GPIO_PORTF_DEN_R |= 0x01;     		//     enable digital I/O on PF0   
  GPIO_PORTF_PCTL_R &= ~0x0000000F; // configure PF0 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       		//     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x01;     		//     enable weak pull-up on PF0
  GPIO_PORTF_IS_R &= ~0x01;     		// (d) PF0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x01;     		//     PF0 is not edges
	GPIO_PORTF_IEV_R &= ~0x01;     		//     PF0 falling edges

	GPIO_PORTF_ICR_R = 0x01;      		// (e) clear flag0
  GPIO_PORTF_IM_R |= 0x01;      		// (f) arm interrupt on PF0
	SW2_Priority = priority;
	priority = (priority & 0x07) << 21;	// NVIC priority bit (21-23)
  //NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF); // clear priority
	NVIC_PRI7_R = (NVIC_PRI7_R | priority); 
  NVIC_EN0_R = 0x40000000;      		// (h) enable interrupt 30 in NVIC 
	LastPF0 = PF0;
}

/** @brief  SW1_Debounce
 *	This thread is added when the user presses a switch
 *  Used for switch debouncing
 *  @param  none
 *  @return none
*/
void SW1_Debounce(void){
	OS_Sleep(10); // sleep for 10ms
	LastPF4 = PF4;
	GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4
	OS_Kill();
}

void SW2_Debounce(void){
	OS_Sleep(10); // sleep for 10ms
	LastPF0 = PF0;
	GPIO_PORTF_ICR_R = 0x01;      // (e) clear flag0
  GPIO_PORTF_IM_R |= 0x01;      // (f) arm interrupt on PF0
	OS_Kill();
}

/** @brief  GPIOPortF_Handler
 *	Executes event task when the user presses the built-in Switch on the TM4C123 Microcontroller
 *  @param  none
 *  @return none
*/
void GPIOPortF_Handler(void){
	if(GPIO_PORTF_RIS_R & 0x10){    // SW1 pressed
		if(LastPF4 == PF4){
			(*SW1_EventThread)();
		}
		GPIO_PORTF_IM_R &= ~0x10;     // disarm interrupt on PF4 
		int status = OS_AddThread(&SW1_Debounce,128,SW1_Priority);
		if(status == 0){ // thread cannot be created
			GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
			GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4
		}
	}
	if(GPIO_PORTF_RIS_R & 0x01){		// SW2 pressed
		if(LastPF0 == PF0){
			(*SW2_EventThread)();
		}
		GPIO_PORTF_IM_R &= ~0x01;     // disarm interrupt on PF0
		int status = OS_AddThread(&SW2_Debounce,128,SW2_Priority);
		if(status == 0){ // thread cannot be created
			GPIO_PORTF_ICR_R = 0x01;      // (e) clear flag4
			GPIO_PORTF_IM_R |= 0x01;      // (f) arm interrupt on PF4
		}
	}
}

//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
/** @brief  OS_AddSW1Task
 *	add a background periodic task and initializes the periodic timer
 *  typically this function receives the highest priority
 *  You are free to select the time resolution for this function
 *  It is assumed that the user task will run to completion and return
 *  This task can not spin, block, loop, sleep, or kill
 *  This task can call OS_Signal  OS_bSignal	 OS_AddThread
 *  This task does not have a Thread ID
 *  @param  pointer to a void/void background function
 *  @param  priority 0 is the highest, 5 is the lowest
 *  @return 1 if successful, 0 if this thread can not be added
*/

int OS_AddSW1Task(void(*task)(void), unsigned long priority){
		SW1_EventThread = task;
		SW1_init(priority); // initialize SW1
		return 1; //add SW1 thread
}

//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW2Task(void(*task)(void), unsigned long priority){
		SW2_EventThread = task;
		SW2_init(priority); // initialize SW2
		return 1; //add SW2 thread
}

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
/** @brief  OS_Id
 *  @param  none
 *  @return thread ID for the currently running thread
*/
unsigned long OS_Id(void){
	return RunPt->Id;
}

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
/** @brief  OS_InitSemaphore
 *  initialize semaphore
 *  @param  pointer to a semaphore
 *  @return none
*/
void OS_InitSemaphore(Sema4Type *semaPt, long value){	//Occurs once at the start
	long status = StartCritical();
	(*semaPt).Value = value;
	semaPt->blockedThreads = 0;
	EndCritical(status);
	
}

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
/** @brief  OS_Wait
 *  decrement semaphore
 *  Lab2 spinlock
 *  Lab3 block if less than zero
 *  @param  pointer to a counting semaphore
 *  @return none
*/
void OS_Wait(Sema4Type *semaPt){ // Called at run time to provide synchronization between threads
#if Lab2
	OS_DisableInterrupts();
	while ((*semaPt).Value <= 0){ // Spin-lock...does nothing until the semaphore counter goes above 0
		OS_EnableInterrupts();			// Allow interrupts to occur while the thread is spinning to not let the software hang
		OS_Suspend();
		OS_DisableInterrupts();
	}
	(*semaPt).Value --;
	OS_EnableInterrupts();
#else
	long status = StartCritical();
	semaPt->Value = semaPt->Value - 1;
	if(semaPt->Value < 0){
		Block(semaPt);
	}
	EndCritical(status);
#endif
}

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
/** @brief  OS_Signal
 *  increment semaphore
 *  Lab2 spinlock
 *  Lab3 wakeup blocked thread if appropriate
 *  @param  pointer to a counting semaphore
 *  @return none
*/
void OS_Signal(Sema4Type *semaPt){ // Called at run time to provide synchronization between threads
#if Lab2
	OS_DisableInterrupts();
 (*semaPt).Value++;
	OS_EnableInterrupts();
#else
	long status = StartCritical();
	semaPt->Value = semaPt->Value + 1;
	if(semaPt->Value <= 0){
		UnBlock(semaPt);
	}
	EndCritical(status);
#endif	
}

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
/** @brief  OS_bWait
 *  sets semaphore to 0
 *  Lab2 spinlock
 *  Lab3 block if less than zero
 *  @param  pointer to a binary semaphore
 *  @return none
*/
void OS_bWait(Sema4Type *semaPt){
#if Lab2
	OS_DisableInterrupts();
	while((*semaPt).Value == 0){
		OS_EnableInterrupts();
		OS_Suspend();
		OS_DisableInterrupts();
	}
	(*semaPt).Value = 0;
	OS_EnableInterrupts();
#else
	OS_DisableInterrupts();
	(*semaPt).Value -=1;
	if(semaPt->Value < 0){
		Block(semaPt);
	}
	OS_EnableInterrupts();
#endif
}

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
/** @brief  OS_bSignal
 *  sets semaphore to 1
 *  Lab2 spinlock
 *  Lab3 wakeup blocked thread if appropriate
 *  @param  pointer to a binary semaphore
 *  @return none
*/
void OS_bSignal(Sema4Type *semaPt){
#if Lab2
	int32_t status = StartCritical();
	(*semaPt).Value = 1;
	EndCritical(status);
#else
	int32_t status = StartCritical();
	semaPt->Value += 1;
	if(semaPt->Value <= 0){
		UnBlock(semaPt);
	}
	EndCritical(status);
#endif
}	

/** @brief  Timer3_MS
 *	Initializes a periodic timer (1 MS)
 *  @param  period given in system time units (12.5ns)
 *  @return none
*/
void Timer3_MS(unsigned long period){long sr;
	sr = StartCritical();
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
  TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER3A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period -1;    // 4) reload value
//	TIMER3_TAILR_R = SysTimeReload -1;
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER3A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0x20000000; // 8) priority 1
// interrupts enabled in the main program after all devices initialized
// vector number 51, interrupt number 35
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable TIMER3A
	EndCritical(sr);
}

// ******** WakeUpThreads ************
/** @brief  WakeUpThreads
 *	Iterates the tcb and decrements sleep counter for sleeping threads
 *  @param  none
 *  @return none
*/
void WakeUpThreads(void){
	for(int i=0;i<NUMTHREADS;i++){
		if((tcbs[i].sleep)&&(tcbs[i].status != -1)){
			tcbs[i].sleep--;
			#if PriScheduler
			if (tcbs[i].sleep == 0) {
				Pri_Available[tcbs[i].priority] = Pri_Available[tcbs[i].priority] + 1;
			}
			#endif 
		}
	}
}

/** @brief  Timer3A_Handler
 *	Updates the systime millisecond timer and executes WakeUpThreads()
 *  @param  none
 *  @return none
*/
void Timer3A_Handler(void){
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER3A timeout
	SystemTime_Ms++;
  WakeUpThreads();                // execute user task
}
// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
/** @brief  OS_Sleep
 *	place this thread into a dormant state
 *  You are free to select the time resolution for this function
 *  OS_Sleep(0) implements cooperative multitasking
 *  @param  number of msec to sleep
 *  @return none
*/
void OS_Sleep(unsigned long sleepTime){
	OS_DisableInterrupts();
	RunPt ->sleep = sleepTime;
	#if PriScheduler
	Pri_Available[RunPt->priority] = Pri_Available[RunPt->priority] - 1;
	#endif
	OS_Suspend();
	OS_EnableInterrupts();
}

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
// Note: there is always atleast one thread running
/** @brief  OS_Kill
 *	kill the currently running thread, release its TCB and stack
 *  @param  none
 *  @return none
*/
void OS_Kill(void){
#if !PriScheduler
	OS_DisableInterrupts();
	UnchainTCB();  //remove current running thread from the circular linked list
	tcbs[RunPt->Id].status = -1; // remember which tcb is free
	OS_Suspend(); //switch to the next task to run
	OS_EnableInterrupts();
	for(;;){}
#else
	OS_DisableInterrupts();
	tcbType *temp, *current;
	if ((RunPt == PriPt[RunPt->priority]) && (RunPt == PriLastPt[RunPt->priority])){ // only one node in the list
		PriPt[RunPt->priority] = 0;
		PriLastPt[RunPt->priority] = 0;
	} else if (RunPt == PriPt[RunPt->priority]){	// Delete the first node in the list
			PriLastPt[RunPt->priority]->Pri_Next = RunPt->Pri_Next;
			PriPt[RunPt->priority] = PriLastPt[RunPt->priority]->Pri_Next;
	} else {	
			temp = current = PriPt[RunPt->priority];
			while (current != RunPt){
				temp = current; current = current->Pri_Next;
		}
		if (RunPt == PriLastPt[RunPt->priority]){ // Delete the last node in the list
			temp->Pri_Next = PriPt[RunPt->priority];
			PriLastPt[RunPt->priority] = temp;
		} else {	// Delete a specific node in the list
			temp->Pri_Next = current->Pri_Next;
		}
	} 
	//RunPt->Pri_Next = 0;
	tcbs[RunPt->Id].status = -1;
	NumThreads--;
	Pri_Total[RunPt->priority] = Pri_Total[RunPt->priority] - 1;
	Pri_Available[RunPt->priority] = Pri_Available[RunPt->priority] - 1;
	OS_Suspend(); //switch to the next task to run
	OS_EnableInterrupts();
	for(;;){}
#endif
}

#define FSIZE 32    // can be any size
#define FIFOSUCCESS 1
#define FIFOFAIL 0 
uint32_t *PutPt;      // index of where to put next
uint32_t *GetPt;      // index of where to get next
uint32_t Fifo[FSIZE];
Sema4Type CurrentFIFOSize;// 0 means FIFO empty, FSIZE means full

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
/** @brief  OS_Fifo_Init
 *	Initialize the Fifo to be empty
 *  the size field in this lab was ignored
 *  @param  Fifo size
 *  @return none
*/
void OS_Fifo_Init(unsigned long size){long sr;  
  sr = StartCritical();        
	PutPt = GetPt = &Fifo[0];
	OS_InitSemaphore(&CurrentFIFOSize, 0);
  EndCritical(sr); 	
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
/** @brief  OS_Fifo_Put
 *	Enter one data sample into the Fifo
 *  Called from the background, so no waiting 
 *  @param  data
 *  @return 1 if data is properly saved, 0 if data not saved because Fifo was full
*/
int OS_Fifo_Put(unsigned long data){
		if(CurrentFIFOSize.Value == FSIZE){
			return FIFOFAIL;
		}
		else{
			*(PutPt) = data; // put
			PutPt++; // place to put next
			if(PutPt == &Fifo[FSIZE]){
				PutPt = &Fifo[0];  // wrap
			}
			OS_Signal(&CurrentFIFOSize);
			return FIFOSUCCESS;
		}
}  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
/** @brief  OS_Fifo_Get
 *	Remove one data sample from the Fifo
 *  Called in foreground, will spin/block if empty
 *  @param  none
 *  @return data
*/
unsigned long OS_Fifo_Get(void){
	unsigned long data;
	OS_Wait(&CurrentFIFOSize);
	if(PutPt == GetPt){
		return FIFOFAIL;
	}
	else{
		data = *(GetPt);
		GetPt++;
		if(GetPt == &Fifo[FSIZE]){
			GetPt = &Fifo[0];
		}
		return data;
	}
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
/** @brief  OS_Fifo_Size
 *	Check the status of the Fifo
 *  @param  none
 *  @return returns the number of elements in the Fifo
*/
long OS_Fifo_Size(void){
	long Fifo_Size;
	Fifo_Size = CurrentFIFOSize.Value;
	return Fifo_Size;
}

/// struct fields for Mailbox
struct MailBox{
	Sema4Type Full;
	Sema4Type Empty;
	unsigned long Data;
};
typedef struct MailBox MailBoxType;
MailBoxType mailbox;
// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
/** @brief  OS_MailBox_Init
 *	Initialize communication channel
 *  @param  none
 *  @return none
*/
void OS_MailBox_Init(void){
	OS_InitSemaphore(&mailbox.Full, 0);
	OS_InitSemaphore(&mailbox.Empty, 1);
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
/** @brief  OS_MailBox_Send
 *	enter mail into the MailBox
 *  This function will be called from a foreground thread
 *  It will spin/block if the MailBox contains data not yet received 
 *  @param  data to be sent
 *  @return none
*/
void OS_MailBox_Send(unsigned long data){
	OS_bWait(&mailbox.Empty);
	mailbox.Data = data;
	OS_bSignal(&mailbox.Full);
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
/** @brief  OS_MailBox_Recv
 *	remove mail from the MailBox
 *  This function will be called from a foreground thread
 *  It will spin/block if the MailBox is empty  
 *  @param  none
 *  @return data received
*/
unsigned long OS_MailBox_Recv(void){
	unsigned long Mailbox_Data;
	OS_bWait(&mailbox.Full);
	Mailbox_Data = mailbox.Data;
	OS_bSignal(&mailbox.Empty);
	return Mailbox_Data;
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
/** @brief  OS_Time
 *  @param  none
 *  @return time in 12.5ns units, 0 to 4294967295
*/
unsigned long OS_Time(void){
	return TIMER0_TAR_R;
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
// this function and OS_Time have the same resolution and precision 
/** @brief  OS_TimeDifference
 *  @param  two times measured with OS_Time
 *  @return time difference in 12.5ns units 
*/
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
	if(start > stop)
		return (start - stop);
	else{
		return (SysTimeReload - stop + start);
	}
}

// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
/** @brief  OS_ClearMsTime
 *  sets the system time to zero
 *  @param  none
 *  @return none 
*/
void OS_ClearMsTime(void){
	SystemTime_Ms  = 0;
}

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
/** @brief  OS_MsTime
 *  reads the current time in msec
 *  @param  none
 *  @return time in ms units 
*/
unsigned long OS_MsTime(void){
	return SystemTime_Ms;
}


void Timer0A_Init(void){long sr;
  sr = StartCritical(); 
  SYSCTL_RCGCTIMER_R |= 0x01;   // 0) activate TIMER0
  TIMER0_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER0_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER0_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = SysTimeReload - 1;    // 4) reload value
  TIMER0_TAPR_R = 0;            // 5) bus clock resolution
  TIMER0_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER0_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x00000000; // 8) priority 1
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN0_R = 1<<19;           // 9) enable IRQ 19 in NVIC
  TIMER0_CTL_R = 0x00000001;    // 10) enable TIMER0A
  EndCritical(sr);
}

void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
}


