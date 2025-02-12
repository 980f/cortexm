//
// Created by andyh on 11/7/24.
//

#include "bluepill.h"


#include "clocks.h" //todo:00 this should not be needed but without it the EXTERNAL_HERTZ below isn't seen by the linker.

//only clocks module should care
const Hertz EXTERNAL_HERTZ = 8'000'000;//geez louise, without 'seeing' the extern directive in clocks.h the compiler forced this to be private.
