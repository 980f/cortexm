/* (C) 2020 Andrew L. Heilveil (github/908f)
This commandline application translates between ARM CortexM bitband values and their byte:bit equivalents.

To build:
gcc bitbandcalculator.cpp
mv a.out whateveryouwishtocallthis

*/

#include <cstdio>
#include "stdlib.h"

#include "bitbanding.h"

unsigned hexify(const char *heximage) {
  return strtoul(heximage, nullptr, 16);
}

unsigned toband(unsigned input, unsigned bitnum) {
  return BandAid(input,bitnum);
}

/** this presumes the input is a valid band address */
unsigned disband(unsigned &address) {
  BandAid aid(address);
  address=aid.byteAddress();
  return aid.bit();
}

int main(int argc, char *argv[]) {
  switch (--argc) {
    case 2: //address  in hexadecimal and bit, print  band address
      printf("BB: %08x \t from:%s.%s", toband(hexify(argv[1]), atoi(argv[2])), argv[1], argv[2]);
      break;
    case 1: //bitband to address and bit
    { //todo: tolerate leading 0x
      unsigned address = hexify(argv[1]);
      unsigned bit = disband(address);
      printf("WA: %08x bit:%d \t from:%s", address, bit,argv[1]);
    }
      break;
    case 0:
      printf("usage:\n one arg is bitband address, two is byte address in hex and bit number\n");
      break;
  }
  printf("\n");

  return 0;
}
