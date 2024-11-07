/**
 * board description for stm32F103 "gumstick" format board
 * (C) 11/7/24 by 980f (Andy Heilveil)
*/

#ifndef BLUEPILL_H
#define BLUEPILL_H

#include <gpio.h>

const unsigned EXTERNAL_HERTZ=8000000;

class Bluepill {
  // Pin ledPin;
  OutputPin led; //low active.

  Bluepill();
  /** toggle it so that we see activity without having to externally track its state */
  void toggleLed();

};



#endif //BLUEPILL_H
