//
// Created by andyh on 11/7/24.
//

#include "blackpill.h"

//A0 is a floating input, switch actively takes it to ground.
//PC13 drives low side of greenled
Blackpill::Blackpill() :led({PC,13},0,PinOptions::slow),key({PA,0},PinOptions::Up,0){}

#include "clocks.h"

//only clocks module should care
const Hertz EXTERNAL_HERTZ = 8'000'000;//geez louise, without 'seeing' the extern directive in clocks.h the compiler forced this to be private.

void Blackpill::toggleLed(){
  led.toggle();//low level access has optimal way of doing this.
}
