#pragma once

//
// Created by andyh on 11/11/24. (C) ANdy Heilveil (github/980f)
//

/**
 * bitbanding is not available in all cortex devices. M0's rarely if ever have it, neither do the superscalars as those also have caches which can't stay coherent as the bitband action would have to be implemented in them as well as in the memory bus controller.
 *
 * The overblown code below was an attempt at constexpr calculations using formal definitions. After fighting for awhile I have kept them for a gui tool.
 */

#include "bitbasher.h"
#include <cstdint>

namespace BandAid {
  struct Banded;//need to predeclare as each class refers to the other
  struct Extractor {
    unsigned byteNumber: 2;
    unsigned offset: 18;
    unsigned alwaysZeroes: 9;
    unsigned space: 3;

    //type punning via offical mechanisms was not compatible with constexpr rules as of c++17, would have worked with c++23.
    constexpr Extractor(uint32_t pattern):
      byteNumber(extractField(pattern, 1, 0)),
      offset(extractField(pattern,19,2)),
      alwaysZeroes(extractField(pattern,28,20)),
      space(extractField(pattern, 31, 29)) {}

    constexpr operator uint32_t () const {
      return Punner<uint32_t,Extractor>(*this).desired;//HAH! This doesn't work on PC architecture
    }

    constexpr Extractor(const Banded &bandAddress);
  };

  struct Banded {

    unsigned unused: 2; //always zero, should be ignored by hardware but don't challenge that.
    unsigned bitnum: 5; //32 bits
    unsigned offset: 18; //2^20 bytes is 2^18 words
    unsigned indicated : 1; //should always be a 1 if this is a valid band address
    unsigned alwaysZeroes : 3; //should always be zero if this is a valid band address
    unsigned space : 3;

    constexpr operator uint32_t() const {
      return bitnum << 2 | offset << 7 | indicated << 25 | space << 29;
    }

    constexpr Banded(Extractor e,unsigned bitNumber):unused(0),bitnum(bitNumber | (e.byteNumber * 8)),offset(e.offset),indicated(1),alwaysZeroes(0),space(e.space){}

    constexpr Banded(uint32_t address,unsigned bitNumber):Banded(Extractor(address),bitNumber){}

    //type punning via offical mechanisms was not compatible with constexpr rules as of c++17, would have worked with c++23.
    constexpr Banded(uint32_t bandaddress=0) : unused(0)
        , bitnum(extractField(bandaddress, 6, 2))
        , offset(extractField(bandaddress, 24, 7))
        , indicated(bitFrom(bandaddress, 25))
        , alwaysZeroes(extractField(bandaddress, 28, 26))
        , space(extractField(bandaddress, 31, 29))
    {}

    constexpr Banded(Banded &&other) = default;
    constexpr Banded(const Banded &other) = default;

  } ;

  constexpr Extractor::Extractor(const Banded &bandAddress):byteNumber(bandAddress.bitnum/8),offset(bandAddress.offset),alwaysZeroes(0),space(bandAddress.space){}

  /** bit band uses the word address as seen by the bus as a byte+bitnumber. To do that the bitnumber has to be placed somewhere in the word address, which is the bitNumber<<2 clause.
  * The word address needs to move out of the way of the bitnumber's shift, and the bitnumber is 5 bits so we get the offset<<7, 7+2+5 term.
  * Bit 25 is apparently an indicator to the bus interface that this is a bitband request and it needs to futz with the address, and the top 3 bits are the cortex M data typing selection. */
  constexpr uint32_t bandFor(uint32_t address,unsigned bitNumber){
    return Banded(address,bitNumber);// e.space<<29 | 1<<25 | e.offset<<7 | bitNumber <<2;
  }

}
