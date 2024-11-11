
/**
 * board description for stm32F103 "gumstick" format board
 * (C) 11/7/24 by 980f (Andy Heilveil)
* el cheapo stm32 board, include one in your main.
*
*/

#pragma once
#define BLUEPILL

#include <gpio.h>

class Bluepill {
  // Pin ledPin;
  OutputPin led; //low active.

  Bluepill();
  //?B6,B7 are pulled up for use with I2C
  //?PB10,11 are pulled up
  /** toggle it so that we see activity without having to externally track its state */
  void toggleLed();
};

