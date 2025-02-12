/* (C) 2020,2024 Andrew L. Heilveil (github/980f)
This commandline application translates between ARM CortexM bitband values and their byte:bit equivalents.

To build:
gcc bitbandcalculator.cpp
mv a.out whateveryouwishtocallthis

*/

#include <cstdio>
#include "stdlib.h"

#include "bitbanding.h"
using namespace BandAid;

unsigned asHex(const char *heximage) {
  return strtoul(heximage, nullptr, 16);
}


const Banded bah(0x40010000,17);
const Banded hum(0x42048030);

int main(int argc, char *argv[]) {
  switch (--argc) {
    case 2: //address  in hexadecimal and bit, print  band address
      printf("BB: %08x \t from:%s.%s", unsigned(Banded(asHex(argv[1]), atoi(argv[2]))), argv[1], argv[2]);
      break;
    case 1:{ //bitband to address and bit
      Banded aid(asHex(argv[1]));
      unsigned wordAddress=Extractor(aid);
      printf("WA: %08x.%u \t from:%s", wordAddress, aid.bitnum, argv[1]);
    }
      break;
    case 0:
      printf("usage:\n one arg is bitband address, two is byte address in hex and bit number:");
      printf("\n %08x %d -> %08x", unsigned(Extractor(bah)),bah.bitnum, unsigned(bah));
      printf("\n %08x -> %08x %u", unsigned(hum), unsigned(Extractor(hum)), hum.bitnum);
      break;
  }
  printf("\n");

  return 0;
}
