//
// (C) Copyright 2021 Andrew Heilveil (github/980f) created on 02/13/21.
//

#include "pinconfigurator.h"

//declare the table
MakeObjectTable(PinConfigurator);

PinInitializer::PinInitializer() {
  ForObjects(PinConfigurator){
    (*it).configure();
  }
}

//we only need to do this once, it handles the pins from all files.
InitStep(InitHardware)
const PinInitializer makem;
