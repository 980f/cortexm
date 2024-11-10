#include "stm32.h"
#include "clocks.h"


Hertz APBdevice::getClockRate() const {
  return clockRate(rbus);
}

//////////////////

#ifdef __linux__  //faking it
  u32 fakeram[2][1024];
#endif
