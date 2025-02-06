#pragma once

//
// Created by andyh on 11/11/24. (C) ANdy Heilveil (github/980f)
//
//#include "stm32.h"

/**
 * bitbanding is not available in all cortex devices. M0's rarely if ever have it, neither do the superscalars as those also have caches which can't stay coherent as the bitband action would have to be implemented in them as well as in the memory bus controller.
 *
 * while initially this could be a namespace it is intended in the future to be a base class for a peripheral that has interesting bits.
 */


constexpr Address bandFor(unsigned byteAddress, unsigned bitnum) {
  //todo:0 overlap lsb of byteaddress with msbs of bitnum. Unitl then trust that input is word address and bit 0..31
 return (BitFielder<2, 18, true>::extract(byteAddress)<<7)| bitnum<<2 | 1<<25 | (byteAddress&(7<<29));
}
  
constexpr bool isBandable(AddressCaster address) {
  auto space=address.space();
  auto mustBeZero=BitFielder<2+18, 9,true>::extract(address.number);
  return (space==2>>1 || space ==4>>1 ) && mustBeZero==0;
}



//the "BandAid" experiment did not work properly with constexpr, unions don't get along with it even when I went to c++20. 
  //struct Banded {
  //  unsigned unused: 2; //always zero, should be ignored by hardware but don't challenge that.
  //  unsigned bitnum: 5; //32 bits
  //  unsigned offset: 18; //2^20 bytes is 2^18 words
  //  unsigned indicated: 1; //should always be a 010 if this is a valid band address
  //  unsigned alwaysZeroes: 3;
  //  unsigned space: 3; //
  //};

  //struct Extractor {
  //  unsigned byteNumber: 2;
  //  unsigned offset: 18;
  //  unsigned alwaysZeroes: 9;
  //  unsigned space: 3;
  //};





