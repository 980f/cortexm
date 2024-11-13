#pragma once

#include "stm32.h"

/** processor oscillator setup and support */

/**
instantiate this in a project specific file such as one that describes the board that this code is going to run on.
It is the rate of the external high speed crystal.
If zero this code will refuse to switch to using it.

The few users that might have a dynamically changing HSE need to write their own code, not alter this module.
*/

extern const Hertz EXTERNAL_HERTZ ;//gcc at c++17 level didn't let this work, got redef despite the weak: __attribute__ ((weak)) =0;

/** @returns a clock rate selected by @param which, which depends upon processor family although many are common
*/
Hertz clockRate(BusNumber which);

/** so far every ST chip I've used has had the same clock generator. If not these will have to be pushed to model specific clock files,  and perhaps have this be a weak version */
constexpr Hertz ahbRate(unsigned ahbPrescale,Hertz feed){
  return feed  >> (ahbPrescale >= 12 ? (ahbPrescale - 6) : (ahbPrescale >= 8 ? (ahbPrescale - 7) : 0));
}

//a different implementation of the above:
constexpr uint8_t AHBPrescTable[16] = {0, 0, 0, 0,   0, 0, 0, 0,   1, 2, 3, 4,   6, 7, 8, 9};//yes, ST left out '5'.
constexpr Hertz ahbRatex(unsigned ahbPrescale,Hertz feed){
  return feed  >> AHBPrescTable[ahbPrescale];
}

constexpr Hertz apbRate(unsigned apbPrescale,Hertz feed){
  return feed >> (apbPrescale >= 4 ? (apbPrescale - 3) : 0);
}

//alternate version of above, clarity vs code size
constexpr uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
constexpr Hertz apbRatex(unsigned apbPrescale,Hertz feed){
  return feed >> APBPrescTable[apbPrescale];
}

//the adc has its own special clock divider
constexpr Hertz adcRate(unsigned adcPrescale,Hertz feed){
  return feed /((1+adcPrescale)*2);
}

/**set system clocks to the fastest possible, @param is which high speed oscillator to use */
void warp9(bool internal);

/** if rate is zero then returns present rate, else sets the divisor as best as possible and returns actual rate*/
Hertz adcClock(Hertz rate=0);

/** this class exists to run clock setup code at a user selectable init level.
    this is done over an init function call so that it can be between construction of other initializing gizmos.
    If we figure out that we can always turn these on first then we may just have a function with a return that inits a static variable that we can InitStep.
 Usage:
    ClockStarter InitStep(InitHardware-100) (false,0,1000);
*/
struct ClockStarter {
  const bool intosc;//hs oscillator selection
  const Hertz coreHertz;
  const Hertz sysHertz;
  /** by declaring an explicit constructor the compiler arranges for it to be called even if we use {} initializer */
  ClockStarter(bool intosc,Hertz coreHertz,Hertz sysHertz);
  /** apply clock settings, called by constructor, but can also call later, such as when coming out of a deep sleep. */
  void go() const ;
};

/** MCO pin configuration (to snoop on internal clock).
*/
void setMCO(unsigned int mode);

