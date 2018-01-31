// main.c
// Partners: Tejasree Ramanuja and Chioma Okorie

// LCD dimenstions 160x128

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "ST7735.h"
#include "ADC.h"
#include "OS.h"
#include "UART.h"
#include "cmdLine.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

#define PF1                     (*((volatile uint32_t *)0x40025008))

//toggle an LED every 100ms
void dummy(void) { 
	UART_OutUDec(OS_ReadPeriodicTime()); 
	OutCRLF();
	PF1 ^= 0x02;
}


void UART_Test(void){
  char string[20];  // global to assist in debugging
  uint32_t n;
	
  OutCRLF();
  UART_OutChar('-');
  UART_OutChar('-');
  UART_OutChar('>');
  while(1){
    UART_OutString("InString: ");
    UART_InString(string,19);
    UART_OutString(" OutString = "); UART_OutString(string); OutCRLF();

    UART_OutString("InUDec: ");    n = UART_InUDec();
    UART_OutString(" OutUDec = "); UART_OutUDec(n); OutCRLF();

  }
}
void LCD_Init (void){
	 ST7735_InitR(INITR_REDTAB);						// LCD Initialization
	 ST7735_FillScreen(0x0000);							// Black screen
	 ST7735_SetCursor (0, 0);
	 ST7735_OutString ("Device 1:");
	 ST7735_SetCursor (0, 9);
	 ST7735_OutString ("Device 2:");
	 ST7735_DrawFastHLine(0, 80, 128, 0xffe0); // Horizontal line that separates the top and bottom display
}

void ADC_Test(void){
	 // ADC Testing
	 uint32_t buffer[9];
	 ADC_Collect (1, 1000, buffer, 9);
	 ST7735_FillScreen (0x0000);
	 for (int i = 0; i < 10; i++){
		 UART_OutUDec (buffer[i]);
		 OutCRLF();
	 }
}

void GPIO_PortF_Init(void){   
	
  SYSCTL_RCGCGPIO_R |= 0x20;     // 1) activate Port F
  while((SYSCTL_PRGPIO_R & 0x20)!=0x20){}; // wait to finish activating     
  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;// 2a) unlock GPIO Port F Commit Register
  GPIO_PORTF_CR_R = 0x0F;        // 2b) enable commit for PF0-PF3    
  GPIO_PORTF_AMSEL_R &= ~0x0F;   // 3) disable analog functionality on PF0-PF3     
  GPIO_PORTF_PCTL_R &= ~0x000FFFF;// 4) configure PF0-PF3 as GPIO
  GPIO_PORTF_DIR_R = 0x0F;       // 5) make PF0-3 output                       
  GPIO_PORTF_AFSEL_R &= ~0x0F;        // 6) disable alt funct on PF0-PF3
  GPIO_PORTF_DEN_R = 0x0F;       // 7) enable digital I/O on PF0-PF3
}

int main(){
	 //PLL_Init(Bus80MHz); 										// Set system clock to 80MHz
	 PLL_Init(Bus50MHz);
   ADC0_InitTimer0ATriggerSeq3(1,0);
	 GPIO_PortF_Init();
	 LCD_Init();
	 UART_Init();															// UART Initialization which includes enabling of interrupts
	 //OS_AddPeriodicThread(dummy,10000000, 1);
	 //cmdLine_Start();
	while(1){
		//ADC_Test();
		cmdLine_Start();
	}
}

// Tests the dimensions of the two separate displays
int main2 (){	
	 ST7735_InitR(INITR_REDTAB);						// LCD Initialization
	 ST7735_FillScreen(0x0000);
	 ST7735_DrawFastHLine(0, 80, 128, 0xffe0);
	 
	// Top 
	 ST7735_SetCursor (0, 0);
	 ST7735_OutString ("Device 1");
	 ST7735_SetCursor (0, 2);
	 ST7735_OutString ("Line 0");
	 ST7735_SetCursor (0, 3);
	 ST7735_OutString ("Line 1");
	 ST7735_SetCursor (0, 4);
	 ST7735_OutString ("Line 2");
	 ST7735_SetCursor (0, 5);
	 ST7735_OutString ("Line 3");
	 ST7735_SetCursor (0, 6);
	 ST7735_OutString ("Line 4");
	
	// Bottom 
	 ST7735_SetCursor (0, 9);
	 ST7735_OutString ("Device 2");
	 ST7735_SetCursor (0, 11);
	 ST7735_OutString ("Line 0");
	 ST7735_SetCursor (0, 12);
	 ST7735_OutString ("Line 1");
	 ST7735_SetCursor (0, 13);
	 ST7735_OutString ("Line 2");
	 ST7735_SetCursor (0, 14);
	 ST7735_OutString ("Line 3");
	 ST7735_SetCursor (0, 15);
	 ST7735_OutString ("Line 4");
	 
	 return 0;
}
