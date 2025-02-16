#pragma once

#define BOARD P103

#include "gpio.h"

/**
olimex/iar stm32-P103 board devices
usart2 is on 9 pin D
usart1 is on ext connector

*/
struct P103_board {
  const OutputPin led; //low active.
  const InputPin button; //also can be the wake_up special function
  //B6,B7 are pulled up for use with I2C
  //PB10,11 are pulled up
  //PA4 spi1 ss
  //PB15 sdcard miso
  constexpr P103_board(): led({PC,12}),
  button({PA,0}){
    /* effectively empty*/
  }
};

const unsigned EXTERNAL_HERTZ=8000000;
