#pragma once

#include "stm32.h"

/** processor oscillator setup and support */

/**
instantiate this in a project specific file:
*/
extern const unsigned EXTERNAL_HERTZ;



//clock rate:
/** @returns a clock rate selected by @param which, which depends upon processor family although many are common
*/
Hertz clockRate(BusNumber which);

constexpr Hertz ahbRate(unsigned ahbPrescale,Hertz feed){
  return feed  >> (ahbPrescale >= 12 ? (ahbPrescale - 6) : (ahbPrescale >= 8 ? (ahbPrescale - 7) : 0));
}

constexpr uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
constexpr Hertz ahbRatex(unsigned ahbPrescale,Hertz feed){
  return feed  >> AHBPrescTable[ahbPrescale];
}


constexpr Hertz apbRate(unsigned apbPrescale,Hertz feed){
  return feed >> (apbPrescale >= 4 ? (apbPrescale - 3) : 0);
}

constexpr uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
constexpr Hertz apbRatex(unsigned apbPrescale,Hertz feed){
  return feed >> APBPrescTable[apbPrescale];
}


constexpr Hertz adcRate(unsigned adcPrescale,Hertz feed){
  return feed /((1+adcPrescale)*2);
}




/**set system clocks to the fastest possible*/
void warp9(bool internal);

/** if rate is zero then returns present rate, else sets the divisor as best as possible and returns actual rate*/
Hertz adcClock(Hertz rate=0);

/** this class exists to run clock setup code at a user selectable init level.
    this is done over an init function call so that it can be between construction of other initializing gizmos.
    If we figure out that we can always turn these on first them we may just have a function with a return that inits a static variable that we can InitStep.
 Usage:
    ClockStarter InitStep(InitHardware-100) (false,0,1000);
*/
struct ClockStarter {
  const bool intosc;//hs oscillator selection
  const Hertz coreHertz;
  const Hertz sysHertz;
  /** by declaring an explicit constructor the compiler arranges for it to be called even if we use {} initializer */
  ClockStarter(bool intosc,Hertz coreHertz,Hertz sysHertz);
};

/** MCO pin configuration (to snoop on internal clock).
*/
void setMCO(unsigned int mode);

