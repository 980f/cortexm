//
// Created by andyh on 11/11/24. (C) ANdy Heilveil (github/980f)
//

#pragma once
/**
 * bitbanding is not available in all cortex devices. M0's rarely if ever have it, neither do the superscalars as those also have caches which can't stay coherent as the bitband action would have to be implemented in them as well as in the memory bus controller.
 *
 * while initially this could be a namespace it is intended in the future to be a base class for a peripheral that has interesting bits.
 */
#include <bitbasher.h>

class BitBandAid {
  static constexpr unsigned int baseMask = 0x000F'FFFF;
  //this bit is set to indicate 'bitband'
  static constexpr unsigned indicator = 0x0200'0000;

  static bool isBandable(unsigned base) {
    if (base & (0x1f << 20)) { //these 5 bits must be zero, they make room for the 5 bits needed to select one of 32 bits.
      return false;
    }

    unsigned space = base >> 29;
    if (space == 2 || space == 4) { //only these two spaces support bit banding
      return true;
    }
    return false;
  }

  static unsigned from(unsigned input, unsigned bitnum) {
    if (!isBandable(input)) return 0; //this value if used should create a hard fault
    unsigned space = split(input, baseMask);
    return space + indicator + (input << 5) + bitnum;
  }

  /** this presumes the input is a valid band address
   * @returns the bit number, alters @param address to be that of the word.
   * so far this is only used for unit tests. */
  unsigned parse(unsigned &address) {
    unsigned space = address >> 29;
    unsigned bitnum = (address >> 2) & 0x1F;
    address >>= 5;
    address &= baseMask;
    address |= (space << 29);
    return bitnum;
  }
};


