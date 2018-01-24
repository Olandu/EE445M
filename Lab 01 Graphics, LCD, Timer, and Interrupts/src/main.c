// main.c
// Partners: Tejasree Ramanuja and Chioma Okorie

// LCD dimenstions 160x128

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "ST7735.h"
#include "ADC.h"
#include "OS.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode


void dummy(void) { 
	
}
int main(){
	  PLL_Init(Bus80MHz); 										// Set system clock to 80MHz
	 ST7735_InitR(INITR_REDTAB);						// LCD Initialization
	 ST7735_FillScreen(0x0000);							// Black screen
	 ST7735_SetCursor (0, 0);
	 ST7735_OutString ("Device 1:");
	 ST7735_SetCursor (0, 9);
	 ST7735_OutString ("Device 2:");
	 ST7735_DrawFastHLine(0, 80, 128, 0xffe0); // Horizontal line that separates the top and bottom display
	 //UART_Init();														// UART Initialization which includes enabling of interrupts
	
	return 0;
}

int main2 (){	// Tests the dimensions of the two separate displays
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

// LCD Initialization
int main1(void){
	ST7735_InitR(INITR_REDTAB);						
	ST7735_FillScreen(0x0000);
	ST7735_DrawFastHLine(0, 80, 128, 0xffe0);
	
	ST7735_SetCursor (0, 0);
	ST7735_OutString ("Device 1");
	ST7735_SetCursor (0, 9);
	ST7735_OutString ("Device 2");
	
	ST7735_Message(1,0,"ADCvalue:",25);
	ST7735_Message(0,0,"ADCvalue:",128);
	return 0;
}
