#include "stdint.h"

//extern "C" {
uint32_t SystemCoreClock = 0;
//used by _hal_rcc, which should be the supplier, not the user!
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
//}


