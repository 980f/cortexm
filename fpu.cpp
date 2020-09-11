//core_cm4 needs a particular device defined first:
#include "stm32f407xx.h"
#include "core_cm4.h" //cmsis special ops
#include "fpu.h"
#include "peripheraltypes.h"
#include "core_cmFunc.h"


/*
0xE000ED88 CPACR coprocessor control
0xE000EF34 FPCCR RW 0xC0000000 Section 4.6.2: Floating-point context control register (FPCCR)
0xE000EF38 FPCAR RW - Section 4.6.3: Floating-point context address register (FPCAR)
0xE000EF3C FPDSCR RW  "default status control"
 fpureg fpscr
 FZ==flush to zero, "should be zero for full ieee754 compliance" but rowley init sets to 1 for 'fast' fp mode
 *
 * */

void fpu_fast(){
  FPSCR |= 3<<24;//FZ and DN
  ControlField (0xE000EF3C,24,2)=3; //FZ and DN
}

void fpu_correct(){
  FPSCR |= 3<<24;//FZ and DN
  ControlField (0xE000EF3C,24,2)=0;
}

void fpu_enable(){
  ControlField (0xE000ED88,20,4)=0xF;//allows access
  //st's manual has DSB preceding the ISB. The order here is from Rowley supplied code.
  __ISB();
  __DSB();
}

void fpu_noisr() {
//if we never use FP hardware in isr's then we can clear two bits:
  ControlField(0xE000EF34, 30, 2) = 0;//don't preserve FPU state on interrupts
  CONTROL &= ~(1 << 2);//on't preserve FPU state on interrupt, why it has to be in two places is beyond me. Plus rowley startup sets it to unconditionally do the stacking
}

void fpu_init(){
  fpu_enable();
  fpu_noisr();
  fpu_fast();
}