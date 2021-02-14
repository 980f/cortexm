//
// (C) Copyright 2020 Andrew Heilveil (github/980f) created on 11/20/20.
//

#pragma once

//#include "gpio.h"  //todo:1 merge gpiof1 and then include generic gpio.h
#include "peripheraltypes.h"
#include "tableofpointers.h"

static const Address io_base {0x4001'4000};


/** used to set the functionality of a pin.
 * the expectation is that a const array of these is defined and iterated over at cstartup.
 *
 * Note: SWCLK is bitnum 30  SWDAT is bitnum 31
 * */
struct PinConfigurator {
  const u8 bitnum;
  /** this guy has irq config , input and output polarity and forces, also use output as OE rather than as signal.
   * for system debug is makes sense to offer forcing a signal until the logic can be fixed, you can access the logical state of a signal even when the pad is forced.
   *
   *
   * */
  const u8 config;//0x40014000 + 4 + 4*bitnum
  const u8 padconfig;//0x4001c000 + 4 + 4*bitnum

  void configure() const {
    //the following should either be sequenced depending upon the change, or done in a critical section as the tiny delay from the signal being change to the pad being changed is ignorable when not interrupted.
    ControlWord(0x4001'4000 + 4 + 8*bitnum) = config;
    ControlWord(0x4001'c000 + 4 + 4*bitnum) = padconfig;
  }

  /** the function select only partially controls what a pin does, the rest is intrinsic to the bitnum.
   * The 0 value still allows for input via the all-at-once read unsure if SIO read also works, probably does. */
  enum Function {
    forNothing=0, forSPI, forUART, forI2C, forTimer, forGPIO, forPIO0, forPIO1, forClock , forUSB
  };

  struct Tweak {
    signed pull=0; signed force=0; unsigned drive_ma=0;//using 0 instead of 2 as they are indistinquishable
  };
  /**
   * @param direction:  -2 schmidt input, -1 plain input, 0 unused , 1 slow output, 2 fast output
   * @param pull: -1 down 0 float 1 up, ignores direction so you can pullup your outputs if you wish
   * @param drive_ma: 2,4,8,12 millamps.
   * @param polarity: 1 for normal, 0 for inverted,
   * @param force: 0 is don't, 1 is active, -1 inactive
   * */
  constexpr PinConfigurator(unsigned bitnum, Function isfor, signed direction, bool polarity, signed pull=0, signed force=0, unsigned drive_ma=2) : //must compute pattern for pad config in the init section to get maximum constness
    bitnum(bitnum)
    ,config(//compute bits in line, will also have runtime modulators for irq

  )
    , padconfig(
    ((direction >= 0) << 7)     //negative direction is inward, positive outward, zero means disable both
    | ((direction > 0) << 6)    //
    | (((drive_ma >> 2) & 3) << 4)  //2 bits, map values 2,4,8,12 to 0,1,2,3  (ie there is a 4ma bit and an 8ma bit)
    | ((pull > 0) << 3)        //pull up
    | ((pull < 0) << 2)        //pull down
    | ((direction < -1) << 1)  //schmidt trigger input
    | ((direction > 1) << 0)   //slew for high speed output, using direction >1
  ) {
  }

  constexpr PinConfigurator(unsigned bitnum, Function isfor, signed direction, bool polarity, Tweak &&tweak) : PinConfigurator(bitnum, isfor, direction, polarity, tweak.pull, tweak.force, tweak.drive_ma){}

};



/** declare one of these to get pins init before main() */
class PinInitializer {
public:
  PinInitializer();
};

//in the macro below the #letter [0] is how you get a single letter constant from a single letter macro operand
#define ConfPin(bitnum, ...) MakeObject(PinConfigurator, CONF ## bitnum, bitnum, __VA_ARGS__);
//in the macro above I used va_args so that we can add constructors with defaults to PinConfigurator

