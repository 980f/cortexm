//
// (C) Copyright 2020 Andrew Heilveil (github/980f) created on 11/20/20.
//


#include "pinconfigurator.h"


/* we could inline a bunch of code from Port here, but we are first going to leverage existing, tested code */
void PinConfigurator::configure() const {
  const Port port= Port(letter);
  port.configure(bitnum,this->opts);
}

void PinConfigurator::doTable(PinConfigurator *table, unsigned count) {
  while (count--) {
    table[count].configure();
  }
}

#if TABLE_OF_POINTERS
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

#endif
