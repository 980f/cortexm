//
// (C) Copyright 2020 Andrew Heilveil (github/980f) created on 11/20/20.
//


#include "pinconfigurator.h"

//declare the table
MakeObjectTable(PinConfigurator);

/* we could inline a bunch of code from Port here, but we are first going to leverage existing, tested code */
void PinConfigurator::configure() const {
  const Port port= Port(letter);
  port.configure(bitnum,*this);
}

PinInitializer::PinInitializer() {
  ForObjects(PinConfigurator){
    (*it).configure();
  }
}

