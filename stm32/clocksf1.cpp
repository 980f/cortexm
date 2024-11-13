// this is the F1 clock stuff

#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#include "clocks.h"
#include "peripheraltypes.h"
#include "gpio.h"
#include "flashf1.h"
#include "systick.h"  //so that we can start it.

//stm32 has an internal RC oscillator:
#define HSI_Hz 8000000

//#ifndef EXTERNAL_HERTZ
//#ifdef EXT_MHz
//#define EXTERNAL_HERTZ (EXT_MHz*1000000)
//#endif
//#endif

struct ClockControl {
  unsigned HSIon : 1;
  volatile unsigned HSIRDY : 1;
  unsigned : 1;
  unsigned HSITrim : 5;
  unsigned HSICal : 8;
  unsigned HSEon : 1;
  volatile unsigned HSErdy : 1;
  unsigned HSEBypass : 1;
  unsigned CSSon : 1;
  unsigned : 4;
  unsigned PLLon : 1;
  volatile unsigned PLLrdy : 1;
  unsigned : 32 - 26;

  unsigned SWdesired : 2;
  volatile unsigned SWactual : 2;
  unsigned ahbPrescale : 4;
  unsigned apb1Prescale : 3; //36MHz max
  unsigned apb2Prescale : 3;
  unsigned adcPrescale : 2;
  unsigned PLLsource : 1;
  unsigned PLLExternalPRE : 1;
  unsigned pllMultiplier : 4;
  unsigned USBPrescale : 1;
  unsigned : 1;
  unsigned MCOselection : 3;
  unsigned : 32 - 27;

  volatile unsigned lsiReady : 1;
  volatile unsigned lseReady : 1;
  volatile unsigned hsiReady : 1;
  volatile unsigned hseReady : 1;
  volatile unsigned pllReady : 1;
  unsigned : 2;
  volatile unsigned cssFaulted : 1;

  unsigned lsiIE : 1;
  unsigned lseIE : 1;
  unsigned hsiIE : 1;
  unsigned hseIE : 1;
  unsigned pllIE : 1;
  unsigned : 3;

  unsigned lsiClear : 1;
  unsigned lseClear : 1;
  unsigned hsiClear : 1;
  unsigned hseClear : 1;
  unsigned pllClear : 1;
  unsigned : 2;
  unsigned cssClear : 1;
  unsigned : 32 - 24;

  unsigned apb2Resets; //accessed formulaically
  unsigned apb1Resets; //accessed formulaically

  unsigned ahbClocks; //accessed formulaically

  unsigned apb2Clocks; //accessed formulaically
  unsigned apb1Clocks; //accessed formulaically

  unsigned LSEon : 1; //32kHz crystal driver
  volatile unsigned LSErdy : 1;
  unsigned LSEbypass : 1;
  unsigned : 5;
  unsigned RTCsource : 2;
  unsigned : 5;
  unsigned RTCenable : 1;
  unsigned BDreset : 1; //reset all battery powered stuff
  unsigned : 32 - 17;

  unsigned LSIon : 1;
  volatile unsigned LSIrdy : 1;
  unsigned : 24 - 2;
  unsigned ResetOccuredFlags : 1; //write a 1 to clear all the other flags
  unsigned : 1;
  volatile unsigned PINresetOccured : 1; //the actual reset pin
  volatile unsigned PORresetOccured : 1;
  volatile unsigned SoftResetOccured : 1;
  volatile unsigned IwatchdogResetOccured : 1;
  volatile unsigned WwatchdogResetOccured : 1;
  volatile unsigned LowPowerResetOccured : 1;

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

  u32 sysClock(unsigned SWcode){
    switch(SWcode) {
    case 0: return HSI_Hz ; //HSI
    case 1: return EXTERNAL_HERTZ;  //HSE, might be 0 if there is none
    case 2: return (pllMultiplier + 2) * (PLLsource ? (PLLExternalPRE ? (EXTERNAL_HERTZ / 2) : EXTERNAL_HERTZ) : HSI_Hz / 2); //HSI is div by 2 before use, and nominal 8MHz for parts in hand.
    default:
      return 0; //defective call argument
    }
  }


  Hertz clockRate(BusNumber bus){
    Hertz rate = sysClock(SWactual);

    switch(bus) {
    case CPU: return rate;
    case AHB1: return ahbRate(ahbPrescale ,rate);
    case APB1: return apbRate(apb1Prescale,rate );
    case APB2: return apbRate(apb2Prescale,rate );
    case ADCbase: return clockRate(APB2) / 2 * (adcPrescale + 1);
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


Hertz clockRate(BusNumber which){
  return theClockControl.clockRate(which);
}

/** stm32 has a feature to post its own clock on a pin, for reference or use by other devices. */
void setMCO(unsigned mode){
  Pin MCO(PA, 8); //depends on mcu family ...

  if(mode >= 4) { //bit 2 is 'enable'
    MCO.FN(Port::PinOptions::Slew::fast); //else we round off the signal.
  } else {
    MCO.DI('F');//set to floating input
  }
  theClockControl.MCOselection = mode;
}
