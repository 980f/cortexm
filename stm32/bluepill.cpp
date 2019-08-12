#include "bluepill.h"

#include "clocks.h"

//only clocks module should care
const u32 EXTERNAL_HERTZ=8000000;//geez lousie, without 'seeing' the extern directive in clocks.h the compiler forced this to be private.

Bluepill::Bluepill():
  ledPin(PC,13),
  led(ledPin){
  /*empty*/
}

void Bluepill::toggleLed(){
  led.toggle();//low level access has optimal way of doing this.
}
