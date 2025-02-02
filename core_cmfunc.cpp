
#include "core_cmFunc.h"

//demo of MREG macro usage
unsigned swapPsp(unsigned whatever){
  unsigned former=PSP;
  PSP=whatever;
  return former;
}

const IrqEnabler IrqEnable;

CREG(control) CONTROL;
CREG(ipsr) IPSR;
CREG(apsr) APSR;
CREG(xpsr) xPSR;

CREG(psp) PSP;
CREG(msp) MSP;

CREG(primask) PRIMASK ;


#if (__CORTEX_M >= 3)
const BasePriority BASEPRI;
CREG(faultmask) FAULTMASK;
#endif /* (__CORTEX_M >= 3) */

#if (__CORTEX_M == 4)
//all special features common to all 4's got converted into individual configuration flags.
#endif

#if __FPU_PRESENT == 1
CREG(fpsr) FPSR;
CFPUREG(fpscr) FPSCR;
#else
unsigned FPSR=0;
unsigned FPSCR=0;
#endif
