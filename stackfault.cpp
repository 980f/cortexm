#include "wtf.h"  //error routine, a place to share a breakpoint for trouble.
#include "cruntime.h"

extern "C" unsigned const __stack_limit__;//created and initialized in linker script
extern "C" void stackFault() {
  unsigned here;
  if (&here <= &__stack_limit__) {//todo:1 should add a few so that we can call wtf without risk
    wtf(99999999);
    generateHardReset();
  }
}

//heapless system
extern "C" void *malloc(unsigned){
  wtf(99999999);
  return nullptr;
}
extern "C" void free(void *){
  wtf(99999999);
}

