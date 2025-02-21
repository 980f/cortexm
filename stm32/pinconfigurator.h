#pragma once
#define PIN_CONFIGURATOR 1
//
// (C) Copyright 2020 Andrew Heilveil (github/980f) created on 11/20/20.
//

#include "gpio.h"
#include "tableofpointers.h"

#define DECLARE_PIN(name,...) MakeObject(PinDeclaration, name,__VA_ARGS__)

#define INPUT_PIN(portIndex,bitnumber,  activeHigh, UDFO) \
  DECLARE_PIN(P##portIndex##bitnumber, *#portIndex, bitnumber ,true, activeHigh,UDFO, false,PinDeclaration::Not_AF,false,PinDeclaration::slow )

#define OUTPUT_PIN(portIndex,bitnumber,  activeHigh, slew, floater) \
DECLARE_PIN(P##portIndex##bitnumber, *#portIndex, bitnumber , false,activeHigh,floater?PinDeclaration::NotPulled:PinDeclaration::Float,false,PinDeclaration::Not_AF,floater,slew);

#define FUNCTION_OUT(portIndex,bitnumber, altfun, activeHigh, slew, floater) \
DECLARE_PIN(P##portIndex##bitnumber, *#portIndex, bitnumber , false,activeHigh,floater?PinDeclaration::NotPulled:PinDeclaration::Float,true,altfun,floater,slew);

#define FUNCTION_INPUT(portIndex,bitnumber, altfun, activeHigh, UDFO) \
DECLARE_PIN(P##portIndex##bitnumber, *#portIndex, bitnumber ,true, activeHigh,UDFO, true,altfun,false,PinDeclaration::slow )

/** will build a table of these, then have a sysinit step that iterators over that table. */
struct PinConfigurator {
  static void doGlobal();
  static void doTable(const PinDeclaration *table,unsigned count);
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
