#pragma once
/**
 * for processors with bitband capability this file is included by peripheraltypes.h.
 * we could just ifdef this content in that file, but it is weird enough to be worth of a separate file.
 */
#include "peripheraltypes.h"
#include "bitbanding.h" 

/** only works for bitbanded item! */
struct ControlBit : ControlWord, BoolishRef {

  constexpr ControlBit(Address sfraddress, unsigned bitnum) : ControlWord(bandFor(sfraddress, bitnum)) {
  }

  // read
  operator bool() const override {
    return item != 0;
  }

  // write
  bool operator=(bool value) const override {
    item = value;
    return value;
  }

  /** for bits declared "rc_w0" in the ST RM */
  bool flagged() const { //todo:1 make this atomic, else only use each either strictly in an isr or not in an isr
    if (item) {
      item = 0;
      return true;
    }
    return false;
  }
};

/** if your bit is in bitband space use this instead of SFRbit */
template<unsigned sfraddress, unsigned bitnum>
struct SFRbandbit : BoolishRef {
  enum {
    bandAddress = bandFor(sfraddress, bitnum)
    , };

  // read
  operator bool() const override {
    return *(reinterpret_cast<volatile unsigned *>(bandAddress));
  }

  // write
  bool operator=(bool value) const override {
    *(reinterpret_cast<unsigned *>(bandAddress)) = value;
    return value;
  }
};
