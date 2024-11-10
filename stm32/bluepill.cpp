<<<<<<< HEAD
//
// Created by andyh on 11/7/24.
//

#include "bluepill.h"

Bluepill::Bluepill() :led({PC,13}){
=======
#include "bluepill.h"

#include "clocks.h"

Bluepill board;

//only clocks module should care
const u32 EXTERNAL_HERTZ=8000000;//geez lousie, without 'seeing' the extern directive in clocks.h the compiler forced this to be private.

Bluepill::Bluepill():
  ledPin(PC,13),
  led(ledPin){
  /*empty*/
}

void Bluepill::toggleLed(){
  led.toggle();//low level access has optimal way of doing this.
>>>>>>> 38af48a193bdaa269537e1f7a37b0db25d5a1b03
}
