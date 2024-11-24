//
// Created by andyh on 2/13/21.
//

#pragma once
#define PIPICO_GPIO_H

#include "peripheraltypes.h"

/**
  * this class manages the nature of a pin, and provides cacheable accessors for the pin value.
  * you may declare each as const, the internals are all const.
  *
  * The first cut only supports the non-qspi port
  */
struct Pin {

  static const Address sio_base {0xD000'0000};
  const unsigned bitnum;//these are so cheap that we const them and create new ones for programs which dynamically assign pins.

//  static constexpr Address statusWord(unsigned bitnum) {
//    return io_base + 8 * bitnum;
//  }
//
//  static constexpr Address controlWord(unsigned bitnum) {
//    return io_base + 4 + 8 * bitnum;
//  }

  constexpr Pin(unsigned bitnum) : bitnum(bitnum) {
    //#nada
  }

///** @returns bitband address for input after configuring as digital input, default pulldown as that is what test equipment induces ;) */
//  const Pin &DI(PinOptions::Puller UDFO = PinOptions::Down) const;
//
///** configure as simple digital output */
//  const Pin &DO(PinOptions::Slew slew = PinOptions::Slew::slow, PinOptions::Puller UDFO = PinOptions::Down) const;
//

/** raw access convenience. @see InputPin for business logic version of a pin */
  INLINETHIS
  operator bool() const { // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
  //slower than following  return ControlBool(statusWord(bitnum), 17);
    return ControlBool(sio_base+0x04,bitnum);
  }

/** @returns pass through @param truth after setting pin to that value.
@see OutputPin for business logic version */
  INLINETHIS // NOLINT(misc-unconventional-assign-operator)
  bool operator=(bool truth) const {
    ControlBool(sio_base+ (truth? 0x14:0x18),bitnum)=1;
    return truth;//#don't reread the pin, nor its actual, keep this as a pass through
  }

  /** normally toggle is added to a class which adds a polarity to the pin, but this chip has that as a hardware feature so we add this function here in the base class of the pin. */
  void toggle() const {
    //slower *this = !*this;  //we can ignore polarity stuff :)
    ControlBool(sio_base+0x1C,bitnum)=1;
  }
};

//GPIO interrupt configuration options.
enum IrqStyle {
  NotAnInterrupt = 0
  , // in case someone forgets to explicitly select a mode
  AnyEdge
  , // edge, either edge, input mode buslatch
  LowActive
  , // level, pulled up
  HighActive
  , // level, pulled down
  LowEdge
  , // edge, pulled up
  HighEdge   // edge, pulled down
};

class ExternalInterrupt {

};


#endif //PIPICODEV_GPIO_H
