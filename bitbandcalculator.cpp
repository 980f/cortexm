/* (C) 2020 Andrew L. Heilveil (github/908f)
This commandline application translates between ARM CortexM bitband values and their byte:bit equivalents.

To build:
gcc bitbandcalculator.cpp
mv a.out whateveryouwishtocallthis

*/

#include <cstdio>
#include "stdlib.h"

static constexpr unsigned int baseaddress = 0x000F'FFFF;

static constexpr unsigned bandindicator = 0x0200'0000;

unsigned hexify(const char *heximage) {
  return strtoul(heximage, nullptr, 16);
}

unsigned toband(unsigned input, unsigned bitnum) {
  unsigned space = input>>29;
  if(space!=2 && space !=4){
    printf("%08X is not an space that can be bitbanded, only lower megabyte of 2000 and 4000 spaces support this\n",input);
    exit(1);
  }  	
  input &= ~(space<<29);//remove space indicator for next test.
	if(input &~ baseaddress){
		printf("%08X is not a base that can be bitbanded, only lower megabyte of 2000 and 4000 spaces support this\n",input|(space<<29));
    exit(2);
	}
	//by the book:
	input &= baseaddress;
	input <<= 5u;
	input |= bitnum << 2u;
	input |= (space<<29);
	input |= bandindicator;
	return input;
}

/** this presumes the input is a valid band address */
unsigned disband(unsigned &address) {
  unsigned space = address >>29;
  unsigned bitnum = (address >> 2) & 0x1F;
  address >>= 5;
  address &= baseaddress;
  address |= (space<<29);
  return bitnum;
}

int main(int argc, char *argv[]) {
  switch (--argc) {
    case 2: //address and bit to bit band address
      printf("BB: %08x", toband(hexify(argv[1]), atoi(argv[2])));
      break;
    case 1: //bitband to address and bit
    { //todo: tolerate leading 0x
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
