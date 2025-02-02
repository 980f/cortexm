#pragma once

#include "eztypes.h"

/** processor oscillator setp and support */

extern const u32 EXTERNAL_HERTZ;

//clock rate:
/** bus is: 0:ahb; 1:apb1; 2:apb2; 3:adc; -1:sysclock;*/
u32 clockRate(unsigned int bus);

/**set system clocks to the fastest possible*/
void warp9(bool internal);
/**MCO pin configuration (to snoop on internal clock)*/
void setMCO(unsigned int mode);

/** this class exists to run clock setup code at a user selectable init level.
 Usage:
ClockStarter InitStep(InitHardware-100) (false,0,1000);
*/
struct ClockStarter {
  const bool intosc;//hs oscillator selection
  const u32 coreHertz;
  const u32 sysHertz;
  ClockStarter(bool intosc,u32 coreHertz,u32 sysHertz);
};

