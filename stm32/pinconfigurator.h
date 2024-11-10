//
// (C) Copyright 2020 Andrew Heilveil (github/980f) created on 11/20/20.
//

#pragma once

#include "gpio.h" 

#include "tableofpointers.h"

/** will build a table of these, then have a sysinit step that iterators over that table. */
struct PinConfigurator : public PinOptions {
  uint8_t letter;
  uint8_t bitnum;

  constexpr PinConfigurator(uint8_t letter, uint8_t bitnum, Dir dir,  Puller udfo, Slew slew=PinOptions::slow, unsigned int altcode=0) : PinOptions(dir, slew, udfo, altcode), letter(letter), bitnum(bitnum) {
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
#define ConfPin(letter, bitnum, ...) MakeObject(PinConfigurator, CONF ## letter ##bitnum, #letter [0],bitnum, __VA_ARGS__);
//in the macro above I used va_args so that we cna add constructors with default to PinConfigurator

#define OutputPin( letter, bitnum, polarity, ... ) \
ConfPin(letter, bitnum, PinOptions::output, PinOptions::slow, PinOptions::Float, 0); \
const LogicalPin letter##bitnum {{P##letter, bitnum}, polarity};

