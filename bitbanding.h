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

namespace BandAid {

  struct Extractor {
    unsigned byteNumber: 2;
    unsigned offset: 18;
    unsigned alwaysZeroes: 9;
    unsigned space: 3;
    constexpr Extractor(unsigned pattern){
      byteNumber=extractField(pattern, 1, 0);
      offset=extractField(pattern,19,2);
      alwaysZeroes=extractField(pattern,28,20);
      space=extractField(pattern, 31, 29);
    }
    struct Banded;
    constexpr Extractor(const Banded &bandAddress);  
  };

  struct Banded {
    unsigned unused: 2; //always zero, should be ignored by hardware but don't challenge that.
    unsigned bitnum: 5; //32 bits
    unsigned offset: 18; //2^20 bytes is 2^18 words
    unsigned indicated: 1; //should always be a 010 if this is a valid band address
    unsigned alwaysZeroes: 3;
    unsigned space: 3; 

    constexpr Banded(unsigned address,unsigned bitNumber){
      Extractor e(address);
      unused=0;
      bitnum=e.byteNumber*8+bitNumber;
      offset=e.offset;
      indicated=1;
      alwaysZeroes=0;
      space=e.space; 
    }

    constexpr Banded(unsigned bandaddress){
      unused=0;//4debug
      bitnum=extractField(bandaddress,6, 2);
      offset=extractField(bandaddress, 24,7); 
      indicated=bitFrom(bandaddress,25);// if not a 1 then this was not a band address
      alwaysZeroes=extractField(bandaddress,28,26);
      space=extractField(bandaddress,31,29);
    };
  };

  constexpr unsigned bandFor(unsigned address,unsigned bitNumber){
    Extractor e(address);
    bitNumber+= e.byteNumber<<3; //in case someone gives byte and bit within byte instead of word and bit within word. Other combos are GIGO.    
    return e.space<<29 | 1<<25 | e.offset<<7 | bitNumber <<2;
  }

}

