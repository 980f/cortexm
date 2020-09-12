#pragma once

#include "eztypes.h"

/** processor oscillator setup and support */

/**
instantiate this in a project specific file:
*/
extern const unsigned EXTERNAL_HERTZ;

using Hertz=unsigned;

//clock rate:
/** @returns a clock rate selected by @param which, which depends upon processor family although many are common
 * stm32: bus is: -1:sysclock; 0:ahb/core; 1:apb1; 2:apb2; 3:adc;
 * lpc13xx: -1:sysclock; 0:core; 1:ahb 2:uart 3 spi 
*/
Hertz clockRate(int which);

/**set system clocks to the fastest possible*/
void warp9(bool internal);

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

