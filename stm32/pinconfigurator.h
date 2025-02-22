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
  static void doTable(const PinDeclaration *table,unsigned count);
  static void doGlobal();
};

