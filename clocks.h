#pragma once

#include "eztypes.h"

/** processor oscillator setup and support */

/**
instantiate this in a project specific file:
*/
extern const u32 EXTERNAL_HERTZ;

//clock rate:
/** @returns a clock rate selected by @param which, which depends upon processor family although many are common
 * stm32: bus is: -1:sysclock; 0:ahb/core; 1:apb1; 2:apb2; 3:adc;
 * lpc13xx: -1:sysclock; 0:core; 1:ahb 2:uart 3 spi 
*/
u32 clockRate(int which);

/**set system clocks to the fastest possible*/
void warp9(bool internal);

/** this class exists to run clock setup code at a user selectable init level.
 Usage:
ClockStarter InitStep(InitHardware-100) (false,0,1000);
*/
struct ClockStarter {
  const bool intosc;//hs oscillator selection
  const u32 coreHertz;
  const u32 sysHertz;
  /** by declaring an explicit constructor the compiler arranges for it to be called even if we use {} initializer */
  ClockStarter(bool intosc,u32 coreHertz,u32 sysHertz);
};

