//
// Created by andyh on 11/7/24.
//

#include "blackpill.h"


#include "clocks.h"

//only clocks module should care
const Hertz EXTERNAL_HERTZ = 12'000'000;//geez louise, without 'seeing' the extern directive in clocks.h the compiler forced this to be private.
