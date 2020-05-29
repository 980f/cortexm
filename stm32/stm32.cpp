#include "stm32.h"
#include "clocks.h"
///** return the bitband address of a bit of a given address*/
//volatile u32 *bandFor(volatile void *dcb, unsigned int bitnum){ //5: 2^5 bits per word. The 0xE0.. and 0x02 .. are cortex-M3 architecture values.
//  return reinterpret_cast <u32 *> ((bitnum << 2) | (((u32) dcb) << 5) | ((((u32) dcb) & 0xE0000000) | 0x02000000));
//}

//constexpr APBdevice::APBdevice(unsigned int stbus, unsigned int slot):
//  bus2(stbus - 1),
//  slot(slot),
//  blockAddress(APB_Block(bus2, slot)),
//  bandAddress(APB_Band(bus2, slot)),
//  //a ternary generated a test and branch around a pair of load via PC
//  rccBitter(bandFor(RCCBASE | ((1u-bus2)<<2u),slot)) //bus 2 is the first of a pair of which bus1 is the second. ST doesn't like consistency.
//{}

void APBdevice::reset() const {
  ControlWord resetter(rccBit(0x0C));
  resetter = 1;
  resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
}

void APBdevice::setClockEnable(bool on) const {
  ControlWord clocker(rccBit(0x18));
  clocker = on;
}

bool APBdevice::isEnabled() const {
  //  (reset on forces the clock off (I think) so we only have to check one bit)
  ControlWord clocker(rccBit(0x18));
  return clocker;
}

void APBdevice::init() const { //frequently used combination of reset then enable clock.
  reset();
  setClockEnable();
}

u32 APBdevice::getClockRate() const {
  return clockRate(bus2 + 1);
}

//////////////////

#ifdef __linux__  //faking it
  u32 fakeram[2][1024];
#endif
