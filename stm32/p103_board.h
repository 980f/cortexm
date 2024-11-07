#pragma once

#ifndef P103_BOARD_H
#define P103_BOARD_H

#include "gpio.h"

/**
olimex/iar stm32-P103 board devices
usart2 is on 9 pin D
usart1 is on ext connector

*/
struct P103_board {
  Pin ledPin;
  OutputPin led; //low active.
  Pin buttonPin;
  InputPin button; //also can be the wake_up special function
  //B6,B7 are pulled up for use with I2C
  //PB10,11 are pulled up
  //PA4 spi1 ss
  //PB15 sdcard miso
  P103_board();
  /** toggle it so that we see activity without having to externally track its state */
  void toggleLed();
};

const unsigned EXTERNAL_HERTZ=8000000;

#endif // P103_BOARD_H
