
/**
 * board description for stm32F103 "gumstick" format board
 * (C) 11/7/24 by 980f (Andy Heilveil)
* el cheapo stm32 board, include this in your main to get you a simple LED.
*
*/

#pragma once
#define BLACKPILL 1

#include "gpio.h"

struct Blackpill {
  
  const OutputPin led; //low active.
  const InputPin key;//will learn level

  //A0 is a floating input, switch actively takes it to ground.
  //PC13 drives low side of greenled
  constexpr  Blackpill() :led({PC,13}),key({PA,0}){}

};
