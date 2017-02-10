#pragma once
/**************************************************************************//**
* core_cmFunc.h
* replaces uses of CMSIS Cortex-M Core Function Access Header File
*
* access to core registers as if they were just regular variables
*/

#include "core_cmInstr.h"

/** couldn't make this happen with a template so:
define a class per M register
declare a const one when you need access, multiple instances are fine.
read and write like an unsigned.
@see core_cmFunc.cpp for example using psp
*/
#define MREG(regname) struct MREG_##regname {\
  operator unsigned () const {\
    unsigned  result;\
  __asm volatile("MRS %0, " #regname "\n" : "=r" (result));\
    return result;\
  }\
  void operator=(unsigned stacktop)const {\
    __asm volatile("MSR " #regname ", %0\n" : : "r" (stacktop));\
  }\
}


/** a readonly instance of MREG. note that the generated name will conflict with an MREG  */
#define MROG(regname) struct MREG_##regname {\
  operator unsigned () const {\
    unsigned  result;\
  __asm volatile("MRS %0, " #regname "\n" : "=r" (result));\
    return result;\
  }\
  void operator=(unsigned stacktop)const=delete;\
}

#define CREG(regname) const MREG_##regname

/** interrupt enable doobers: */
#define CPSI(iorf) struct CPSI_##iorf {\
  void operator =(bool enable)const {\
    if(enable){\
      __asm volatile ("cpsie " #iorf);\
    } else {\
      __asm volatile ("cpsid " #iorf);\
    }\
  }\
}


/* since duplicate declarations cost nothing, not even extra rom as the code inlines, it would probably be easier to
 * not declare all of these here, but declare within each using cpp file.
*/
extern const CPSI(i) IrqEnable; //IrqEnable=1 or 0

extern const MREG(control) CONTROL;
//read only instances:
extern const MROG(ipsr) IPSR;
extern const MROG(apsr) APSR;
extern const MROG(xpsr) xPSR;

extern const MREG(psp) PSP;
extern const MREG(msp) MSP;

extern const MREG(primask) PRIMASK ;

/** @returns previous psp value while setting it to @param whatever */
unsigned swapPsp(unsigned whatever);

#if __CORTEX_M >= 3
extern const CPSI(f) FIQenable; //=1 or 0

extern const struct BasePriority {
  operator unsigned () const {
    unsigned  result;
    __asm volatile("MRS %0, basepri_max\n" : "=r" (result));
    return result;
  }
  void operator=(unsigned stacktop)const {
    __asm volatile("MSR basepri, %0\n" : : "r" (stacktop));
  }
}BASEPRI;

extern const MREG(faultmask) FAULTMASK;
#endif /* (__CORTEX_M >= 3) */


#if (__CORTEX_M == 4)

#if __FPU_PRESENT == 1
extern const MREG(fpsr) FPSR;
#else
extern unsigned FPSR;
#endif

#endif /* (__CORTEX_M == 4) */
