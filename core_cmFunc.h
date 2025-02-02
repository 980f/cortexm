#pragma once

/**************************************************************************//**
* core_cmFunc.h
* replaces uses of CMSIS Cortex-M Core Function Access Header File
*
* access to core registers as if they were just regular variables
 *
 * Ignores whether the feature is present on a device, just don't try to use it if not.
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
  void operator|=(unsigned stacktop)const {\
    *this= unsigned(*this)|stacktop;\
  }\
  void operator&=(unsigned stacktop)const {\
    *this= unsigned(*this)&stacktop;\
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

//experiment in templating MSR access: foiled again, will have to write code executes a register's constant.
//template <unsigned sysm> struct MROG {
//  enum {reader=sysm|0xF3EF'8000, writer=sysm|0xF380'8800};
//  [[naked]]
//  operator unsigned () const  {
//  __asm volatile(".word reader\nbx lr\n");
//    //return;
//  }
//  void operator=(unsigned stacktop)const=delete;
//};

//extern MROG<8> theMSP;



#define CREG(regname) const MREG_##regname

/** interrupt enable doober: 
*   There is only one type, the FIQ stuff is never present on a cortex M, which is a shame but was dropped so that we could have tail chaining.
*
* Note that at present this has a high runtime cost compared to the function call syntax, this was created so that we could substitute a boolean for the interrupt enable and that might be too expensive to keep this around.
*/

struct IrqEnabler {
  
  inline void operator =(bool enable) const {
    if(enable){
      __asm volatile ("cpsie i");
    } else {
      __asm volatile ("cpsid i");
    }
  }
};


/* since duplicate declarations cost nothing, not even extra rom as the code inlines, it would probably be easier to
 * not declare all of these here, but declare within each using cpp file.
*/
extern const IrqEnabler IrqEnable; //IrqEnable=1 or 0

extern const MREG(control) CONTROL;
//read only instances:
extern const MROG(ipsr) IPSR;
extern const MROG(apsr) APSR;
extern const MROG(xpsr) xPSR;

//assembler doesn't like these two when cross compiling, ignore unless happens with arm compiler:
extern const MREG(psp) PSP;
extern const MREG(msp) MSP;

extern const MREG(primask) PRIMASK ;

/** @returns previous psp value while setting it to @param whatever */
unsigned swapPsp(unsigned whatever);


//leftover from some other ARM family, not a cortex M ever: extern const CPSI(f) FIQenable; //=1 or 0

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

#define FPUREG(regname) struct FPUREG_##regname {\
  operator unsigned () const {\
    unsigned  result;\
  __asm volatile("VMRS %0, " #regname "\n" : "=r" (result));\
    return result;\
  }\
  void operator=(unsigned stacktop)const {\
    __asm volatile("VMSR " #regname ", %0\n" : : "r" (stacktop));\
  }\
    void operator|=(unsigned stacktop)const {\
    *this= unsigned(*this)|stacktop;\
  }\
  void operator&=(unsigned stacktop)const {\
    *this= unsigned(*this)&stacktop;\
  }\
}

#define CFPUREG(regname) const FPUREG_##regname

#if __FPU_PRESENT == 1
extern const MREG(fpsr) FPSR;//normal space
extern const FPUREG(fpscr) FPSCR;
#endif
