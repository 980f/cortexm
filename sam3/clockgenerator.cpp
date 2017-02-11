#include "clockgenerator.h"

#include "peripheraltypes.h"
#include "peripheralband.h"
#include "feature.h"
#include "clocks.h" //this provides the generic functions

using namespace SAM;

static const unsigned SysBase=0x400E0000;

const unsigned SamMaxMhz=84; //someday get this from a device specific file.
const unsigned XtalMhz=12; //someday get this from a board specific file.

/** the slow clock is nominally this value */
const unsigned slowkHz=32;


enum SysEntry {
  SMC,MATRIX,PMC,UART, //CHIPID is tucked into the tail end of the uart
  EFC0,EFC1,
  PIOA,PIOB,PIOC,  //gpio ports,
  RESET_ETC  //Resets and other little stuff
};

constexpr unsigned SysItem(SysEntry which){
  return SysBase+(which<<9);
}

///

/* features at PMC:
 * 0:SysclockEnable
 * 1:peripheralClockEnable + UTMIclock
*/

enum ClockInterrupt {
  MOSCXTS=0,  //Main XTAL Oscillator
  LOCKA=1, // PLL-A Lock Interrupt
  MCKRDY=3, // Master Clock Ready
  LOCKU=6,   // UTMI PLL Lock
  PCK0=8,  // Programmable Clock Ready x
  PCK1,  // Programmable Clock Ready x
  PCK2,  // Programmable Clock Ready x

  MOSCSELS=16, // Main Oscillator Selection Status
  MOSCRCS=17,  // Main On-Chip RC Status
  CFDEV=18,   // Clock Failure Detector Event Interrupt Mask
  //and the next are read only:
  CFDS=19, // Clock Failure Detector Status , failure of the main on-chip RC oscillator clock is detected.
  FOS=20   // Clock Failure Detector Fault Output Status
};

template<unsigned whichOf3> void clockoutPut( bool enable){
  Feature<SysItem(PMC),whichOf3+PCK0> cen=enable;
}


/** @param prescale is 3 bits, 111 for /3 else log2(some power of 2)
 * @param source is slow, main, pllA, Upll. if Upll then the uppl/2 bit is set herein, else it is cleared.
*/
void masterClock(unsigned prescale, unsigned source){
  *atAddress(0x400E0430)=(prescale<<4)+source+((source==3)<<13);
}

/** @param divider is linear divide of pll output, 0= no clock, 1=no division, max=255.
 * @param multiplier:  (MULA + 1)/DIVA.  multiplier==0 is a disable. (max 2047 is not achievable electrically )
 * The "pll is stable" signal is delayed @param locktimer*8 slowclock cycles from the write of this register.
 * the pll input freq must be in the range 1..40 MHz, so the typical 12 is fine.
 * the pll output freq must be 80 to 240 MHz, @12 that is a minimum of 7 (put six)

 */
void pllA(unsigned multiplier, unsigned divider, unsigned locktimer){
  *atAddress(0x400E0428)= divider | (locktimer<<8) | ((multiplier-1)<<16);
}

unsigned clockRate(int which){
  switch (which) {
  case -1: //systick  
    return clockRate(0)/8;
  case 0:
    return 84000000;
  }
  return 0;
}


void warp9(bool internal){
  if(internal){
  //set source to fastRC, and set that to 12MHz
} else {
  //set source to xtal,
//set PLL
//spin until PLL settles.

//    pllA(7,1,63);
//    0b001;

//    *atAddress(0x400E0420)= (0x37<<16);
}
}

