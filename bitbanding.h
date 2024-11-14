//
// Created by andyh on 11/11/24. (C) ANdy Heilveil (github/980f)
//

#pragma once
/**
 * bitbanding is not available in all cortex devices. M0's rarely if ever have it, neither do the superscalars as those also have caches which can't stay coherent as the bitband action would have to be implemented in them as well as in the memory bus controller.
 *
 * while initially this could be a namespace it is intended in the future to be a base class for a peripheral that has interesting bits.
 */
class BandAid {
  struct Banded {
    unsigned unused: 2; //always zero, should be ignored by hardware but don't challenge that.
    unsigned bitnum: 5; //32 bits
    unsigned offset: 18; //2^20 bytes is 2^18 words
    unsigned indicated: 1; //should always be a 010 if this is a valid band address
    unsigned space: 6; //
  };

  /** to parse assign to pattern then read fields from banded */
  union ParseBanded {
    unsigned pattern;
    Banded b;
    ParseBanded(unsigned p) : pattern(p) {}
  } parser;

  struct Extractor {
    unsigned byteNumber: 2;
    unsigned offset: 18;
    unsigned alwaysZeroes: 6;
    unsigned space: 6;
  };

  /** to parse assign to pattern then read fields from banded */
  union ParseWord {
    unsigned pattern;
    Extractor e;
    ParseWord(unsigned p) : pattern(p) {}
  } extractor;

public:
  /** two args is byte address and bitnumber */
  BandAid(unsigned byteAddress, unsigned bitNumber): parser(byteAddress), extractor(byteAddress) {
    // parser.b.space = extractor.e.space;
    parser.b.indicated = 1;
    parser.b.offset = extractor.e.offset;
    parser.b.bitnum = (extractor.e.byteNumber * 8) + bitNumber; //bits per byte
    parser.b.unused = 0;
  }

  /** one arg is bitband address. */
  BandAid(unsigned bandit): parser(bandit), extractor(bandit) {
    // extractor.e.space = parser.b.space;
    extractor.e.alwaysZeroes = 0;
    extractor.e.offset = parser.b.offset;
    extractor.e.byteNumber = parser.b.bitnum / 8; //bits per byte
  }

  unsigned asBanded() const {
    return parser.pattern;
  }

  unsigned byteAddress() const {
    return extractor.pattern;
  }

  unsigned bit() const {
    return parser.b.bitnum;
  }

  /** primary purpose is to pack. So:
   * unsigned bitbanded=BandAid(unsigned bytish,unsigned bitnum);
   */
  operator unsigned() const {
    return asBanded();
  }

  bool inband() const {
    return extractor.e.alwaysZeroes == 0;
  }

  bool bandable() const {
    return extractor.e.space == 0b001000 || extractor.e.space == 0b010000; //so far all banding has been ram and peripheral space, not sure it is allowed anywhere else. This also does not check processor type!
  }
};


