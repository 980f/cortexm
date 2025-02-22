
/**
 * board description for stm32F103 "gumstick" format board
 * (C) 11/7/24 by 980f (Andy Heilveil)
* el cheapo stm32 board, include this in your main to get you a simple LED.
*
*/

#pragma once
#define BLACKPILL 1

#include "pinconfigurator.h"

//OUTPUT_PIN(C,13,false,PinDeclaration::slow,false);
//INPUT_PIN(A,0,true,PinDeclaration::Up);

extern const PinDeclaration PC13;
extern const PinDeclaration PA0;

struct Blackpill {
  
  const Pin led; //low active.
  const Pin key;//will learn level

  //A0 is a floating input, switch actively takes it to ground.
  //PC13 drives low side of greenled
  constexpr  Blackpill() :led(PC13),key(PA0){}

};
