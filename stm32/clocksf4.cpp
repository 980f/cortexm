#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"


#if !defined(EMBEETLE) || DEVICE ==407 || DEVICE == 411

#include "clocks.h"
#include "stm32.h"
#include "peripheraltypes.h"
#include "gpiof4.h"

#include "systick.h"  //so that we can start it.

//stm32F4 internal RC oscillator:
#define HSI_Hz 16000000
//hack for quick switch during development
#ifndef EXT_MHz
#if DEVKIT==1
#define EXT_MHz 25
#else
#define EXT_MHz 12
#endif
#endif
//the following wasn't getting linked when in main.cpp (needed 'extern' in header
//constexpr unsigned EXTERNAL_HERTZ = 1000000 * EXT_MHz;
constexpr unsigned MAX_HERTZ = 168000000;

struct OscControl {
  ControlBit on;
  ControlBit ready;

  OscControl(unsigned bitnum) : on(RCCBASE, bitnum), ready(RCCBASE, bitnum + 1) {}

  bool operator=(bool beOn) {
    on = beOn;
    while (!ready) {
      //use an interrupt to deal with clock failure!
    }
    return true;
  }
};

OscControl HSI(0);
OscControl HSE(16);
OscControl PLL(24);

#define PLLCFGR RCCBASE+4
ControlField PLLM(PLLCFGR, 0, 6);
ControlField PLLN(PLLCFGR, 6, 9);
ControlField PLLP(PLLCFGR, 16, 2);
ControlBit PLLsource(PLLCFGR, 22);
ControlField PLLQ(PLLCFGR, 24, 4);

#define RCCCC RCCBASE+8
ControlField selector(RCCCC, 0, 2);
ControlField selected(RCCCC, 2, 2);

void waitForClockSwitchToComplete() {
  while (selected != selected) {
    //could check for hopeless failures,
    //or maybe toggle an otherwise unused I/O pin.
  }
}

void switchClockTo(unsigned code) {
  selected = code;
  while (selected != code) {
    //could check for hopeless failures,
    //or maybe toggle an otherwise unused I/O pin.
  }
}

ControlField ahbPrescale(RCCCC, 4, 4);
ControlField apb1Prescale(RCCCC, 10, 3);
ControlField apb2Prescale(RCCCC, 13, 3);
//more as needed


//flash control functions for F40x parts
void setFlash4Clockrate(Hertz hz) {
  //the below relies on truncating divide, do not round or ceil.
  ControlField(FLASHBASE, 0, 3) = hz / 30'000'000;//full voltage. For lesser voltage change the denominator
}

void switchToInteral() {
  HSI = 1;
  switchClockTo(0);
}

void switchToExternal(bool crystal) {
  if (crystal) {
    HSE = 1;
  }
  //not this guy's job to turn the HSE off.
  switchClockTo(1);
}

void switchToPll() {
  PLL = 1;
  switchClockTo(2);
}



//  /**set all clocks for their fastest possible, given the reference source of internal 8MHz else external RefOsc.
//  * todo:M check hardware identification registers to determine max speeds.
//  */
//  void maxit(bool internal){
//    int pllDesired;
//unsigned selection=0;
//    //ensure the one we are switching to is on, not our job here to turn off the unused one.
//    if(internal||EXTERNAL_HERTZ==0) {
//      selector=0;
//      pllDesired = 2 * 8; // *8 net as there is a /2 in the hardware between HSI and PLL
//    } else {
//      selector = 2;
//      pllDesired = MAX_HERTZ / EXTERNAL_HERTZ; //#div by zero warning OK if no external osc on particular board.
//    }
//    pllDesired -= 2; //from literal value to code that goes into control register
//
//    ahbPrescale = 2;
//    apb1Prescale = 4;
//    apb2Prescale = 2;
//
//    if(pllDesired != pllMultiplier || PLLsource == internal) { //then need to turn pll off to make changes
//      switchToInteral();
//
//      //now is safe to muck with PLL
//      PLL = 0;//turn it off before dicking with its settings.
//
//      //todo: see if we are actually going to be slowing down, if so don't set the flash waits until after new clock is established.
//      setFlash4Clockrate(sysClock(2)); //must execute before the assignment to SWdesired
//
//      PLLsource = !internal;
//
//      pllMultiplier = pllDesired;
//
//      switchToPll();
//    }
//  } /* maxit */


Hertz sysClock(unsigned int SWcode) {
  switch (SWcode) {
  case 0:
    return HSI_Hz; //HSI
  case 1:
    return EXTERNAL_HERTZ;  //HSE, might be 0 if there is none
  case 2: {
    float whatamess = PLLsource ? EXTERNAL_HERTZ : HSI_Hz;
    whatamess /= PLLM;
    whatamess *= PLLN;
    whatamess /= (2 + 2 * PLLP);
    return whatamess;
  }
  default:
    return 0; //defective call argument
  }
}

static const ControlField adcPrescaler{0x300+0x04+0x4001'2000,16,2};

Hertz clockRate(BusNumber rbus) {//
  u32 rate = sysClock(selected);
  switch (rbus) {
  case CPU: // processor clock
    return rate;
  case AHB1:
  case AHB2:
  case AHB3: //3 AHB's share the same clock
    return ahbRate(ahbPrescale, rate);
  case APB1://4 is a spacer for programming convenience, nomninally 'apb0'
    return apbRate(apb1Prescale, rate);
  case APB2:
    return apbRate(apb2Prescale, rate);
  default:
    return 0; //should blow up on user.
  }
} /* clockRate */

/** @returns actual rate, if @param rate is zero then sets the divisor as best as possible before returning actual rate
 * chooses highest rate lower than asked for, but lowest if none are lower than asked for.
 *
 * */
Hertz adcClock(Hertz rate){
  Hertz feed=clockRate(APB2);
  if(rate>0){
    for(unsigned choices=0;choices<4;++choices){
      Hertz possible=adcRate(choices,feed);
      if(possible<=rate){
        adcPrescaler=choices;
        return possible;
      }
    }
    adcPrescaler=3;//slow as we can
  }
  return adcRate(adcPrescaler,feed);
}

/** stm32 has a feature to post its own clock on a pin, for reference or use by other devices. */
void setMCO(unsigned int mode) {
#if 0
Bits 26:24 MCO: Microcontroller clock output
Set and cleared by software.
0xx: No clock
100: System clock (SYSCLK) selected
101: HSI clock selected
110: HSE clock selected
111: PLL clock divided by 2 selected
Note: This clock output may have some truncated cycles at startup or during MCO clock
source switching.
When the System Clock is selected to output to the MCO pin, make sure that this clock
does not exceed 50 MHz (the maximum IO speed).

Bits 22:21 MCO1: Microcontroller clock output 1
Set and cleared by software. Clock source selection may generate glitches on MCO1. It is
highly recommended to configure these bits only after reset before enabling the external
oscillators and PLL.
00: HSI clock selected
01: LSE oscillator selected
10: HSE oscillator clock selected
11: PLL clock selected
  //Bits 26:24 MCO1PRE: MCO1 prescaler
Set and cleared by software to configure the prescaler of the MCO1. Modification of this
prescaler may generate glitches on MCO1. It is highly recommended to change this
prescaler only after reset before enabling the external oscillators and the PLL.
0xx: no division
100: division by 2
101: division by 3
110: division by 4
111: division by 5
#endif
//
}

#endif //embeetle builder issue

#pragma clang diagnostic pop