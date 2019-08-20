#pragma once

#define BLUEPILL_H

#include "gpio.h"

/**
el cheapo stm32 board, a base class for your project
*/
struct Bluepill {
  Pin ledPin;
  OutputPin led; //low active.
  //?B6,B7 are pulled up for use with I2C
  //?PB10,11 are pulled up
  Bluepill();
  /** toggle it so that we see activity without having to externally track its state */
  void toggleLed();
};

extern Bluepill board;//there can be only one. (if there are none then don't include the .cpp)

