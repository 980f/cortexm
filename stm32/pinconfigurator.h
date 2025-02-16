#pragma once
#define PIN_CONFIGURATOR 1
//
// (C) Copyright 2020 Andrew Heilveil (github/980f) created on 11/20/20.
//

#include "gpio.h"
#include <cstdint>


/** will build a table of these, then have a sysinit step that iterators over that table. */
struct PinConfigurator {
  char letter;
  uint8_t bitnum;
  Port::PinOptions opts;

  // constexpr PinConfigurator(char letter, uint8_t bitnum, Port::PinOptions::Dir dir,  Port::PinOptions::Puller udfo, Port::PinOptions::Slew slew=Port::PinOptions::slow, Port::PinOptions::Altfunc altcode=Port::PinOptions::Altfunc::SYS_AF) : letter(letter), bitnum(bitnum), opts(dir,  udfo, slew, altcode) {
  //   //#nada, we wish to statically construct with data, no function calling (constexpr)
  // }

  void configure() const;
  static void doTable(PinConfigurator *table,unsigned count);
};


#if 0 //not yet:

#include "tableofpointers.h"

//in the macro below the #letter [0] is how you get a single letter constant from a single letter macro operand
#define ConfPin(letter, bitnum, ...) MakeObject(PinConfigurator, CONF ## letter ## bitnum, #letter [0], bitnum, __VA_ARGS__);
//in the macro above I used va_args so that we cna add constructors with default to PinConfigurator

#define OutputPin( letter, bitnum, polarity, ... ) \
ConfPin(letter, bitnum, PinOptions::output, PinOptions::slow, PinOptions::Float, 0); \
const LogicalPin letter##bitnum {{P##letter, bitnum}, polarity};

/** declare one of these to get pins init before main() */
class PinInitializer {
public:
  PinInitializer();
};


#endif
