#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#include "clocks.h"
#include "peripheral.h"
#include "gpio.h"
#include "flashcontrol.h"
#include "systick.h"  //so that we can start it.

//stm32 has an internal RC oscillator:
#define HSI_Hz 8000000

struct ClockControl {
  unsigned int HSIon : 1;
  volatile unsigned int HSIRDY : 1;
  unsigned int : 1;
  unsigned int HSITrim : 5;
  unsigned int HSICal : 8;
  unsigned int HSEon : 1;
  volatile unsigned int HSErdy : 1;
  unsigned int HSEBypass : 1;
  unsigned int CSSon : 1;
  unsigned int : 4;
  unsigned int PLLon : 1;
  volatile unsigned int PLLrdy : 1;
  unsigned int : 32 - 26;

  unsigned int SWdesired : 2;
  volatile unsigned int SWactual : 2;
  unsigned int ahbPrescale : 4;
  unsigned int apb1Prescale : 3; //36MHz max
  unsigned int apb2Prescale : 3;
  unsigned int adcPrescale : 2;
  unsigned int PLLsource : 1;
  unsigned int PLLExternalPRE : 1;
  unsigned int pllMultiplier : 4;
  unsigned int USBPrescale : 1;
  unsigned int : 1;
  unsigned int MCOselection : 3;
  unsigned int : 32 - 27;

  volatile unsigned int lsiReady : 1;
  volatile unsigned int lseReady : 1;
  volatile unsigned int hsiReady : 1;
  volatile unsigned int hseReady : 1;
  volatile unsigned int pllReady : 1;
  unsigned int : 2;
  volatile unsigned int cssFaulted : 1;

  unsigned int lsiIE : 1;
  unsigned int lseIE : 1;
  unsigned int hsiIE : 1;
  unsigned int hseIE : 1;
  unsigned int pllIE : 1;
  unsigned int : 3;

  unsigned int lsiClear : 1;
  unsigned int lseClear : 1;
  unsigned int hsiClear : 1;
  unsigned int hseClear : 1;
  unsigned int pllClear : 1;
  unsigned int : 2;
  unsigned int cssClear : 1;
  unsigned int : 32 - 24;

  unsigned int apb2Resets; //accessed formulaically
  unsigned int apb1Resets; //accessed formulaically

  unsigned int ahbClocks; //accessed formulaically

  unsigned int apb2Clocks; //accessed formulaically
  unsigned int apb1Clocks; //accessed formulaically

  unsigned int LSEon : 1; //32kHz crystal driver
  volatile unsigned int LSErdy : 1;
  unsigned int LSEbypass : 1;
  unsigned int : 5;
  unsigned int RTCsource : 2;
  unsigned int : 5;
  unsigned int RTCenable : 1;
  unsigned int BDreset : 1; //reset all battery powered stuff
  unsigned int : 32 - 17;

  unsigned int LSIon : 1;
  volatile unsigned int LSIrdy : 1;
  unsigned int : 24 - 2;
  unsigned int ResetOccuredFlags : 1; //write a 1 to clear all the other flags
  unsigned int : 1;
  volatile unsigned int PINresetOccured : 1; //the actual reset pin
  volatile unsigned int PORresetOccured : 1;
  volatile unsigned int SoftResetOccured : 1;
  volatile unsigned int IwatchdogResetOccured : 1;
  volatile unsigned int WwatchdogResetOccured : 1;
  volatile unsigned int LowPowerResetOccured : 1;

  /**set all clocks for their fastest possible, given the reference source of internal 8MHz else external RefOsc.
  * todo:M check hardware identification registers to determine max speeds.
  */
  void maxit(bool internal){
    int pllDesired;

    //ensure the one we are switching to is on, not our job here to turn off the unused one.
    if(EXTERNAL_HERTZ==0 ||internal) {
      HSIon = 1;
      pllDesired = 2 * 8; // *8 net as there is a /2 in the hardware between HSI and PLL
    } else {
      HSEon = 1;
      PLLExternalPRE = 0; // whether to divide down external clock before pll.
      pllDesired = 72000000 / EXTERNAL_HERTZ; //#div by zero warning OK if no external osc on particular board.
      // ...72MHz:F103's max.
    }
    pllDesired -= 2; //from literal value to code that goes into control register
    ahbPrescale = 0; // 0:/1
    adcPrescale = 2; // 2:/6  72MHz/6 = 12MHz  64MHz/6=10+ Mhz, both less than 14Mhz.
    apb1Prescale = 4; // 4:/2
    apb2Prescale = 0; // 0:/1
    if(pllDesired != pllMultiplier || PLLsource == internal) { //then need to turn pll off to make changes
      HSIon = 1;
      while(!HSIRDY) {
        /*spin, forever if chip is broken*/
      }
      SWdesired = 0;   //0:HSI
      waitForClockSwitchToComplete();
      //now is safe to muck with PLL
      PLLon = 0;
      //flash wait states must be goosed up: >48 needs 2
      PLLsource = !internal;
      pllMultiplier = pllDesired;
      PLLon = 1;
      while(!PLLrdy) {
        /*spin, forever if chip is broken*/
      }
      setFlash4Clockrate(sysClock(2)); //must execute before the assignment to SWdesired
      SWdesired = 2; //2:PLL
      waitForClockSwitchToComplete();
    }
  } /* maxit */

  void waitForClockSwitchToComplete(){
    while(SWdesired != SWactual) {
      //could check for hopeless failures,
      //or maybe toggle an otherwise unused I/O pin.
    }
  }

  u32 sysClock(unsigned int SWcode){
    switch(SWcode) {
    case 0: return HSI_Hz ; //HSI
    case 1: return EXTERNAL_HERTZ;  //HSE, might be 0 if there is none
    case 2: return (pllMultiplier + 2) * (PLLsource ? (PLLExternalPRE ? (EXTERNAL_HERTZ / 2) : EXTERNAL_HERTZ) : HSI_Hz / 2); //HSI is div by 2 before use, and nominal 8MHz for parts in hand.
    default:
      return 0; //defective call argument
    }
  }

  u32 clockRate(unsigned int bus){
    u32 rate = sysClock(SWactual);

    switch(bus) {
    case ~0U: return rate;
    case 0: return rate >> (ahbPrescale >= 12 ? (ahbPrescale - 6) : (ahbPrescale >= 8 ? (ahbPrescale - 7) : 0));
    case 1: return rate >> (apb1Prescale >= 4 ? (apb1Prescale - 3) : 0);
    case 2: return rate >> (apb2Prescale >= 4 ? (apb2Prescale - 3) : 0);
    case 3: return clockRate(2) / 2 * (adcPrescale + 1);
    default:
      return 0; //should blow up on user.
    }
  } /* clockRate */
};

//there is only one of these:
static soliton(ClockControl, RCCBASE);

////////////////////
// support for cortexm/clocks.h:

void warp9(bool internal){
  theClockControl.maxit(internal);
}

u32 clockRate(int which){
  return theClockControl.clockRate(which);
}

/** stm32 has a feature to post its own clock on a pin, for reference or use by other devices. */
void setMCO(unsigned int mode){
  Pin MCO(PA, 8); //depends on mcu family ...

  if(mode >= 4) { //bit 2 is 'enable'
    MCO.FN(Portcode::fast); //else we round off the signal.
  } else {
    MCO.configureAs(4);//set to floating input
  }
  theClockControl.MCOselection = mode;
}


#pragma clang diagnostic pop