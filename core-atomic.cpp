#include "core-atomic.h"


#if __has_include(<atomic>)
#include <atomic>  //todo:1 actually use it rather than our own cortexm3.s file.
#endif

/** cortex M atomic operations
 * todo:1 see if compiler vintage includes atomic.h
 */
#if __linux__  //fake the asm routines to get a link on PC platform,
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
// assembler file will supply code
#endif // if HOST_SIM


