#pragma once

#ifndef BLUEPILL_H
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

//experimenting with putting this in the header file, only the main() should include a board file and having this here reduces some $ifdef'ing.
extern Bluepill board;
#endif // P103_BOARD_H
