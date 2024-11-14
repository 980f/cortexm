/* (C) 2020,2024 Andrew L. Heilveil (github/980f)
This commandline application translates between ARM CortexM bitband values and their byte:bit equivalents.

To build:
gcc bitbandcalculator.cpp
mv a.out whateveryouwishtocallthis

*/

#include <cstdio>
#include "stdlib.h"

#include "bitbanding.h"

unsigned asHex(const char *heximage) {
  return strtoul(heximage, nullptr, 16);
}

const BandAid bah(0x40010000,17);
const BandAid hum(0x42048030);

int main(int argc, char *argv[]) {
  switch (--argc) {
    case 2: //address  in hexadecimal and bit, print  band address
      printf("BB: %08x \t from:%s.%s", unsigned(BandAid(asHex(argv[1]), atoi(argv[2]))), argv[1], argv[2]);
      break;
    case 1: //bitband to address and bit
    { BandAid aid(asHex(argv[1]));
      printf("WA: %08x.%u \t from:%s", aid.byteAddress(), aid.bit(), argv[1]);
    }
      break;
    case 0:
      printf("usage:\n one arg is bitband address, two is byte address in hex and bit number:");
      printf("\n %08x %d -> %08x", bah.extractor.pattern,bah.parser.b.bitnum, unsigned(bah));
      printf("\n %08x -> %08x %u", hum.parser.pattern, hum.byteAddress(), hum.bit());
      break;
  }
  printf("\n");

  return 0;
}
