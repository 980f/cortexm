#include "stm32.h"
#include "clocks.h"


void APBdevice::reset() const {
  ControlWord resetter(rccBit(resetOffset));
  resetter = 1;
  resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
}

void APBdevice::setClockEnable(bool on) const {
  ControlWord clocker(rccBit(clockOffset));
  clocker = on;
}

bool APBdevice::isEnabled() const {
  //  (reset on forces the clock off (I think) so we only have to check one bit)
  ControlWord clocker(rccBit(clockOffset));
  return clocker;
}

void APBdevice::init() const { //frequently used combination of reset then enable clock.
  reset();
  setClockEnable();
}

Hertz APBdevice::getClockRate() const {
  return clockRate(rbus + 1);
}

//////////////////

#ifdef __linux__  //faking it
  u32 fakeram[2][1024];
#endif
