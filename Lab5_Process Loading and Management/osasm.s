;/*****************************************************************************/
; OSasm.s: low-level OS commands, written in assembly                       */
; Runs on LM4F120/TM4C123
; A very simple real time operating system with minimal features.
; Daniel Valvano
; January 29, 2015
;
; This example accompanies the book
;  "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
;  ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
;
;  Programs 4.4 through 4.12, section 4.2
;
;Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
;    You may use, edit, run or distribute this file
;    as long as the above copyright notice remains
; THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
; OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; For more information about my classes, my research, and my books, see
; http://users.ece.utexas.edu/~valvano/
; */

        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

        EXTERN  RunPt            ; currently running thread
		EXTERN  NextRunPt        ; point to next thread to run
        EXPORT  OS_DisableInterrupts
        EXPORT  OS_EnableInterrupts
        EXPORT  StartOS
		EXPORT  PendSV_Handler
		EXPORT  SVC_Handler

		EXTERN OS_Id
		EXTERN OS_Kill
		EXTERN OS_Sleep
		EXTERN OS_Time
		EXTERN OS_AddThread


OS_DisableInterrupts
        CPSID   I
        BX      LR


OS_EnableInterrupts
        CPSIE   I
        BX      LR


PendSV_Handler
    CPSID   I                  ; Prevent interruption during context switch
    PUSH    {R4-R11}           ; Save remaining regs r4-11 
    LDR     R0, =RunPt         ; R0=pointer to RunPt, old thread
    LDR     R1, [R0]		   ; RunPt->stackPointer = SP;
    STR     SP, [R1]           ; save SP of process being switched out
	
	LDR		R1, =NextRunPt
	LDR		R1,[R1]			   ; R1 = NextRunPt
    ;LDR     R1, [R1,#4]        ; 6) R1 = RunPt->next
    STR     R1, [R0]           ;    RunPt = R1
	
    LDR     SP, [R1]           ; 7) new thread SP; SP = RunPt->sp;
    POP     {R4-R11}           ; restore regs r4-11 
    CPSIE   I				   ; tasks run with I=0
    BX      LR                 ; Exception return will restore remaining context 

StartOS
    LDR     R0, =RunPt         ; currently running thread
    LDR     R2, [R0]           ; R2 = value of RunPt
    LDR     SP, [R2]           ; new thread SP; SP = RunPt->stackPointer;
    POP     {R4-R11}           ; restore regs r4-11
    POP     {R0-R3}            ; restore regs r0-3
    POP     {R12}
	ADD		SP,SP, #4
    POP     {LR}               ; discard LR from initial stack
	ADD		SP, SP, #4		   ; discard PSR
    CPSIE   I                  ; Enable interrupts at processor level
    BX      LR                 ; start first thread


SVC_Handler
	LDR R12,[SP,#24] 			; Return address
	LDRH R12,[R12,#-2] 			; SVC instruction is 2 bytes
	BIC R12,#0xFF00 			; Extract ID in R12
	LDM SP,{R0-R3} 				; Get any parameters
	
	PUSH {LR}
	LDR LR, = Return
	CMP R12, #0
	BEQ OS_Id
	CMP R12, #1
	BEQ OS_Kill
	CMP R12, #2
	BEQ OS_Sleep
	CMP R12, #3
	BEQ OS_Time
	CMP R12, #4
	BEQ OS_AddThread
	
Return 	POP {LR}
	STR R0,[SP] 				; Store return value
	BX LR 						; Return from exception

    ALIGN
    END
