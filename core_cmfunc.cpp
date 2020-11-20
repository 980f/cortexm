
#include "core_cmFunc.h"

//demo of MREG macro usage
unsigned swapPsp(unsigned whatever){
  unsigned former=PSP;
  PSP=whatever;
  return former;
}

const CPSI_i IrqEnable;

CREG(control) CONTROL;
CREG(ipsr) IPSR;
CREG(apsr) APSR;
CREG(xpsr) xPSR;

CREG(psp) PSP;
CREG(msp) MSP;

CREG(primask) PRIMASK ;


#if (__CORTEX_M >= 3)
const CPSI_f FIQenable; //=1 or 0
const BasePriority BASEPRI;
CREG(faultmask) FAULTMASK;

#endif /* (__CORTEX_M >= 3) */

#if (__CORTEX_M == 4)

#if __FPU_PRESENT == 1
CREG(fpsr) FPSR;
CFPUREG(fpscr) FPSCR;
#else
unsigned FPSR=0;
unsigned FPSCR
#endif

#endif
