//
// Created by andyh on 11/7/24.
//

//this double created the PinDeclarations as it is also included in the application main 
#include "blackpill.h"

OUTPUT_PIN(C,13,false,PinDeclaration::slow,false);
INPUT_PIN(A,0,true,PinDeclaration::Up);

#include "clocks.h"

//only clocks module should care
const Hertz EXTERNAL_HERTZ = 12'000'000;//geez louise, without 'seeing' the extern directive in clocks.h the compiler forced this to be private.
