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

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode


//toggle an LED every 100ms
void dummy(void) { 
	
}

//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
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
	 ADC_Collect (1, 10000, buffer, 9);
	 ST7735_FillScreen (0x0000);
	 for (int i = 0; i < 10; i++){
		 UART_OutUDec (buffer[i]);
		 OutCRLF();
	 }
}
int main(){
	 //PLL_Init(Bus80MHz); 										// Set system clock to 80MHz
	 PLL_Init(Bus50MHz);
   ADC0_InitTimer0ATriggerSeq3(0,1000);	
	 LCD_Init();
	 UART_Init();			// UART Initialization which includes enabling of interrupts
	 OS_AddPeriodicThread(dummy,5000000, 1);
	while(1){
		ADC_Test();
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
