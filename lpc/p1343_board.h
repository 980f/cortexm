#pragma once

#include "gpio.h" // to declare buttons and lamps.

namespace LPC {

class P1343devkit {
public:
  /** most common external clock, happens to be the same as the internal RC */
  enum {ExpectedClock = 12000000};
  const LPC::InputPin<2,9> button;//={2,9};
  //other button is 'wakeup'
  const LPC::InputPin<1,4> wakeup;
  //reset button is pio0/0, if reset functionality is defeated.
  //pin 1/0 is bootup select, avoid hanging anything important on it.
  const LPC::OutputPin<3,0> led0;//={3, 0};
  const LPC::OutputPin<3,1> led1;//={3, 1};
  const LPC::OutputPin<3,2> led2;//={3, 2};
  const LPC::OutputPin<3,3> led3;//={3, 3};
  const LPC::OutputPin<2,4> led4;//={2, 4};
  const LPC::OutputPin<2,5> led5;//={2, 5};
  const LPC::OutputPin<2,6> led6;//={2, 6};
  const LPC::OutputPin<2,7> led7;//={2, 7};
  //changing the above from Output's to OutputPins added a bunch of vtables. Code mushroomed while data shrunk a little bit.
  //it trades ram init'ed consts initialized with constructor code for vtables. The vtables should have been removable by linker ...

  //parallel access to the leds
  const GpioOutputField lownib={3, 3, 0};
  const GpioOutputField highnib={2, 7, 4};

  /** set lamps as an 8-bit number, not particularly swift in execution since they are scattered about the i/o space*/
  unsigned operator =(unsigned lamp )const;
  /** set led by ordinal.*/
  const BoolishRef &led(unsigned which)const ;
  /** invert state of one led */
  void toggleLed(unsigned which=0)const ;
};
}

#if 0
UEXt connector pinout:
  3.3  gnd
  txd  rxd
  scl  sda
  miso mosi
  sck  ssel
#endif
