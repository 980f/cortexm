//
// (C) Copyright 2020 Andrew Heilveil (github/980f) created on 11/20/20.
//

#pragma once
#include "gpiof4.h"  //todo:1 merge gpiof1 and then include generic gpio.h

#include "tableofpointers.h"

/** will build a table of these, then have a sysinit step that iterators over that table. */
struct PinConfigurator : public PinOptions {
  uint8_t letter;
  uint8_t bitnum;

  constexpr PinConfigurator(uint8_t letter, uint8_t bitnum,Dir dir, Slew slew, Puller udfo, unsigned int altcode) : PinOptions(dir, slew, udfo, altcode), letter(letter), bitnum(bitnum) {
  }

  void configure() const ;

};

/** declare one of these to get pins init before main() */
class PinInitializer {
public:
  PinInitializer();
};

#define ConfPin( letter,  bitnum, ... ) MakeObject(PinConfigurator, letter ##bitnum, #letter [0],bitnum, __VA_ARGS__);


