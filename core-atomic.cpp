#include "core-atomic.h"
/** cortex M atomic operations
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
// assembler file will supply code

#endif // if HOST_SIM


