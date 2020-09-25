#pragma once 

#include "wtf.h"  //error routine, a place to share a breakpoint for trouble.

extern unsigned const __stack_limit__;//created and initialized in linker script
void stackFault() {
  unsigned here;
  if (&here <= &__stack_limit__) {//todo: should add a few so that we can call wtf without risk
    wtf(99999999);
    generateHardReset();
  }
}

