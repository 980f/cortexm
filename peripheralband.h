#pragma once
/**
 * for processors with bitband capability this file is included by peripheraltypes.h.
 * we could just ifdef this content in that file, but it is weird enough to be worth of a separate file.
 */
#include "peripheraltypes.h"

const Address PeripheralBand {0x4200'0000};//bandFor(PeripheralBase)

//this guy moves around too much, live with gratuitous warnings: #include "../ignoresignwarnings.h"


// constexpr u32 bandShift(u32 *byteOffset){
//   return reinterpret_cast<u32>(byteOffset)<<5;
// }
// constexpr u32 bandShift(u32 byteOffset){
//   return byteOffset<<5;
// }


/** many, but not all, cortex devices put peripheral control registers in the 0x4000 space, and bitband all of that to 0x4200.
 * "bitband" is ARM's term for mapping each bit of the lower space into a 32bit word in the bitband region.
This replaces a 3-clock operation that is susceptible to interruption into a one clock operation that is not. That is important if an ISR is modifying the same control word as main thread code.
*/
static constexpr unsigned int BandGroup = 0xE000'0000;//bit banding only applies to 0000 0000 through 000F FFFF in the 2000 and 4000 regions.
static constexpr unsigned BandBit = 0x0200'0000;

constexpr Address bandShift(Address byteOffset) {
  //5: 2^5 bits per 32 bit word. we expect a byte address here, so that one can pass values as read from the stm32 manual.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbraced-scalar-init"  //# leave braces in case Address becomes a real class
  return {byteOffset << 5U};
}

/** @return bitband address for given bit (default 0) of @param byte address.
this assumes that the byte address ends in 00, which all of the ones in the st manual do.
*/
constexpr Address bandFor(Address byteAddress, unsigned bitnum = 0) {
  return {(bitnum << 2U) | bandShift(byteAddress) | (byteAddress & BandGroup) | BandBit};
}

#pragma clang diagnostic pop

/** @return bitband address for given bit (default 0) of @param byte address.
this assumes that the byte address ends in 00, which all of the ones in the st manual do.
*/
constexpr unsigned bandit(unsigned byteAddress, unsigned bitnum = 0){
  //bit gets shifted by 2 as the underlying mechanism inspects the 32bit word address only, doesn't see the 2 lsbs of the address.
  //5: 2^5 bits per 32 bit word. we expect a byte address here, so that one can pass values as read from the stm32 manual.
  //0xE000 0000: stm32 segments memory into 8 512M blocks, banding is within a block
  //0x0200 0000: indicates this is a bitband address
  //bit to lsbs of address |  byteaddress shifted up far enouhg for address space to go away | restore address space | bitband indicator.
  return ((bitnum << 2) | bandShift(byteAddress) | (byteAddress & 0xE0000000) | 0x02000000);
}


/** only works for bitbanded item! */
struct ControlBit : public ControlWord, BoolishRef {

  constexpr ControlBit(Address sfraddress, unsigned bitnum) : ControlWord(bandFor(sfraddress, bitnum)) {
  }

  // read
  inline operator bool() const override {
    return item != 0;
  }

  // write
  bool operator=(bool value) const override {
    item = value;
    return value;
  }

  /** for bits declared "rc_w0" in the ST RM */
  bool flagged() const {//todo:1 make this atomic, else only use each either strictly in an isr or not in an isr
    if (item) {
      item = 0;
      return true;
    } else {
      return false;
    }
  }
};

/** if your bit is in bitband space use this instead of SFRbit */
template<unsigned sfraddress, unsigned bitnum>
struct SFRbandbit : public BoolishRef {
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
//
// /** @return bitband address for given bit (default 0) of @param byte address.
// this assumes that the byte address ends in 00, which all of the ones in the st manual do.
// */
// constexpr volatile unsigned *bandFor(unsigned byteAddress, unsigned bitnum = 0){
//   return atAddress ((bitnum << 2) | bandShift(byteAddress) | (byteAddress & 0xE0000000) | 0x02000000);
// }

// /** @return bitband address for given bit (default 0) of @param byte address.
// this assumes that the byte address ends in 00, which all of the ones in the st manual do.
// */
// constexpr volatile unsigned *bandAddress(unsigned byteAddress, unsigned bitnum = 0){
//   return bandFor(byteAddress,bitnum);
// }

// const unsigned PeripheralBand(0x4200'0000);

