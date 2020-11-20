#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#include "clocks.h"
#include "stm32.h"
#include "peripheraltypes.h"
#include "gpiof4.h"

#include "systick.h"  //so that we can start it.

//stm32F4 internal RC oscillator:
#define HSI_Hz 16000000
//hack for quick switch during development
#ifndef EXT_MHz
#error "!you must define EXT_MHz, to 0 if you don't have one"
#endif
//the following wasn't getting linked when in main.cpp
constexpr unsigned EXTERNAL_HERTZ = 1000000 * EXT_MHz;
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
  ControlField(FLASHBASE, 0, 3) = hz / 16'000'000;//full voltage. For lesser voltage change the denominator
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

/** there are 3 ranges, 4 steps per range, first two ranges logarithmic the last linear, so we are not going to be fancy */
constexpr Hertz MsiCutoff[] = {
  100'000, 200'000, 400'000, 800'000,
  1'000'000,  2'000'000,  4'000'000,  8'000'000,
  16'000'000,  24'000'000,  32'000'000,  48'000'000,
};

constexpr unsigned MsiRange(Hertz desired) {
  for (unsigned code = countof(MsiCutoff); code-- > 0;) {
    if (MsiCutoff[code] >= desired) {
      return code;
    }
  }
  return 0;
}

/* senselessly complex:
 * one bit tells us which of two 4 bit field sets the expected value
 * if in pllmode then the accuracy depends upon LSE being 32kHz so we actually should multiply by LSE/32kHz.
 * */
Hertz msiActual(){
  ControlBit selector(RCCBASE,3);
  ControlField ifLow(RCCBASE,4,4);
  ControlField ifHigh(RCCBASE+0x94,8,4);
  unsigned code=selector?ifHigh:ifLow;
  return MsiCutoff[code];// if pll then multiply by LSE/32kHz
}

Hertz sysClock(unsigned int SWcode) {
  switch (SWcode) {
    case 0:
      return  msiActual();
    case 1:
      return HSI_Hz; //HSI
    case 2:
      return EXTERNAL_HERTZ;  //HSE, might be 0 if there is none
    case 3: {
      float whatamess = PLLsource ? EXTERNAL_HERTZ : HSI_Hz;
      whatamess *= PLLN;
      whatamess /= PLLM * (2 + 2 * PLLP);
      return whatamess;
    }
    default:
      return 0; //defective call argument
  }
}

static const ControlField adcPrescaler{0x300 + 0x04 + 0x4001'2000, 16, 2};

Hertz clockRate(BusNumber rbus) {//
  u32 rate = sysClock(selected);
  switch (rbus) {
    case CPU: // processor clock
      return rate;
    case AHB1:
    case AHB2:
    case AHB3: //3 AHB's share the same clock
      return ahbRate(ahbPrescale, rate);
    case APB1:
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
Hertz adcClock(Hertz rate) {
  Hertz feed = clockRate(APB2);
  if (rate > 0) {
    for (unsigned choices = 0; choices < 4; ++choices) {
      Hertz possible = adcRate(choices, feed);
      if (possible <= rate) {
        adcPrescaler = choices;
        return possible;
      }
    }
    adcPrescaler = 3;//slow as we can
  }
  return adcRate(adcPrescaler, feed);
}

/** stm32 has a feature to post its own clock on a pin, for reference or use by other devices. */
void setMCO(unsigned int mode) {
//todo: implement F4 version
  Pin MCO(PA, 8); //depends on mcu family ... same for both 103 and 407
  //PC,9 is a second one.
//
//  if(mode >= 4) { //bit 2 is 'enable'
//    MCO.FN(Portcode::fast); //else we round off the signal.
//  } else {
//    MCO.configureAs(4);//set to floating input
//  }
//  theClockControl.MCOselection = mode;
}

#pragma clang diagnostic pop
