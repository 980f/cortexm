//
// (C) Copyright 2020 Andrew Heilveil (github/980f) created on 11/20/20.
//

#include "eztypes.h"
#include "pinconfigurator.h"

//arguments must be valid, but the related object will never be touched in any way.
MakeObjectTable(PinDeclaration,0,0,true,true, PinDeclaration::Puller::NotPulled, false, PinDeclaration::Not_AF, false
  , PinDeclaration:: Slew:: slow);

void PinConfigurator::doTable(const PinDeclaration *table, unsigned count) {
  //terse but inefficient way of doing this:
  while (count--) {
    table[count].configure();
  }
}

#if PIN_CONFIGURATOR >=1
InitDo(InitHardware)
#endif
void PinConfigurator::doGlobal() {
  PinConfigurator::doTable(ObjectTableStart(PinDeclaration),ObjectTableSize(PinDeclaration));
}
