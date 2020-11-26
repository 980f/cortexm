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

  constexpr PinConfigurator(uint8_t letter, uint8_t bitnum, Dir dir, Slew slew, Puller udfo, unsigned int altcode) : PinOptions(dir, slew, udfo, altcode), letter(letter), bitnum(bitnum) {
    //#nada, we wish to statically construct with data, no function calling (constexpr)
  }

  void configure() const;
};

/** declare one of these to get pins init before main() */
class PinInitializer {
public:
  PinInitializer();
};

//in the macro below the #letter [0] is how you get a single letter constant from a single letter macro operand
#define ConfPin(letter, bitnum, ...) MakeObject(PinConfigurator, letter ##bitnum, #letter [0],bitnum, __VA_ARGS__);
//in the macro above I used va_args so that we cna add constructors with default to PinConfigurator

