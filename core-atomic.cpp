#include "core-atomic.h"
/** cortex M atomic operations
* only include this if your processor has ldrex and strex instructions.
 */

#if __has_include(<atomic>)
#include <atomic>  //todo:1 actually use it rather than our own cortexm3.s file.
#endif


/** cortex M atomic operations */
#if __linux__   //create stubs, perhaps someday use <atomic> to do a real implementation.
bool atomic_increment(unsigned &alignedDatum){
  ++alignedDatum;
  return false;
}

bool atomic_decrement(unsigned &alignedDatum){
  --alignedDatum;
  return false;
}

bool atomic_decrementNotZero(unsigned &alignedDatum){
  if(alignedDatum) {
    --alignedDatum;
    return true;
  }
  return false;
}

bool atomic_incrementNotMax(unsigned &alignedDatum){
  if(alignedDatum != 0xffffffff) {
    ++alignedDatum;
  }
  return false;
}

bool atomic_setIfZero(unsigned &alignedDatum, unsigned value){
  if(alignedDatum == 0){
    alignedDatum=value;
  }
  return false;
}
#else // real code

__attribute__((naked))
bool atomic_increment(unsigned & counter){
  asm volatile(
    "\nldrex r1,[r0]"
    "\nadd r1,r1,#1"
    "\nstrex r2,r1,[r0]"
    "\nmov r0,r2"
    "\nbx lr"
      );
}

#endif // if HOST_SIM


