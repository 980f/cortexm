
#include <cstdio>
#include "stdlib.h"

static const unsigned int spacely = 0xFC00'0000;

static const unsigned bandindicator = 0x0200'0000;

unsigned hexify(const char *heximage) {
  return strtoul(heximage, nullptr, 16);
}

unsigned toband(unsigned input, unsigned bitnum) {
  unsigned space = input & spacely;
  input &= ~spacely;
  input <<= 5u;
  input |= bitnum << 2u;
  input |= space;
  input |= bandindicator;
  return input;
}

unsigned disband(unsigned &address) {
  unsigned space = address & spacely;
  address &= ~spacely;
  address &= ~bandindicator;
  unsigned bitnum = (address >> 2) & 0x1F;
  address >>= 5;
  address &= ~0x3;
  address |= space;
  return bitnum;
}

int main(int argc, char *argv[]) {

  switch (--argc) {
    case 2: //address and bit to bit band address
      printf("BB: %08x", toband(hexify(argv[1]), atoi(argv[2])));
      break;
    case 1: //bitband to address and bit
    {
      unsigned address = hexify(argv[1]);
      unsigned bit = disband(address);
      printf("WA: %08x bit:%d", address, bit);
    }
      break;
    case 0:
      printf("usage:\n one arg is bitband address, two is word address and bit number\n");
      break;
  }
  printf("\n");

  return 0;
}
