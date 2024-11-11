//
// Created by andyh on 11/7/24.
//

#include "bluepill.h"

Bluepill::Bluepill() :led({PC,13}){}

#include "clocks.h"

//only clocks module should care
const Hertz EXTERNAL_HERTZ = 8'000'000;//geez louise, without 'seeing' the extern directive in clocks.h the compiler forced this to be private.

void Bluepill::toggleLed(){
  led.toggle();//low level access has optimal way of doing this.
}
