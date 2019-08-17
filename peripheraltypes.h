#pragma once

#include "bitbanger.h"
#include "boolish.h"
#include "eztypes.h"

/**
 *  types used for declaration of peripherals.
 *
 *  The SFR* template classes have a peculiar usage pattern, caused by trying to make all related code be generated inline.
 *  At hardware module definition time one creates a typedef for each field rather than an instance.
 *  At each place of use one creates an instance and then assigns to that instance to write a value or simply reference that instance for a read.
 *  This precludes extern'ing global instances which then must take up real data space (const so not a big cost) and then be accessed via an extra level of indirection compared to what the 'create local instance' can result in.
 *  If we could create a global instance in a header file but get just one actual object created we would do that.
 * Volatile is used to keep the compiler from optimizing away accesses that have physical side effects.
I am working on replacing *'s with &'s, its a statisitcal thing herein as to which is better.
*/

//inline void setBit(unsigned  address, unsigned bit){
//  *atAddress(address)|= 1<<bit;
//}

//inline void clearBit(unsigned  address, unsigned  bit){
//  *atAddress(address) &= 1<<bit;
//}

///** ensure a 0:1 transition occurs on given bit. */
//inline void raiseBit(unsigned  address, unsigned  bit){
//  clearBit(address, bit);
//  setBit(address, bit);
//}

/** for a private single instance block */
#define soliton(type, address) type& the##type = *reinterpret_cast<type*>(address);

/** the following are mostly markers, but often it is tedious to insert the 'volatile' and dangerous to leave it out. */
typedef volatile u32 SFR;
/** marker for non-occupied memory location */
typedef const u32 SKIPPED;

/** marker for an address, will eventually feed into a *reinterpret_cast<unsigned *>() */
using Address = unsigned;
#define Ref(address) *reinterpret_cast<volatile Address*>(address)

/** most cortex devices follow arm's suggestion of using this block for peripherals */
const Address PeripheralBase(0x40000000);  //1<<30

const Address PeripheralBand(0x42000000);

/** many, but not all, cortex devices put peripheral control registers in the 0x4000 space, and bitband all of that to 0x4200.
 * "bitband" is ARM's term for mapping each bit of the lower space into a 32bit word in the bitband region.
This replaces a 3-clock operation that is susceptible to interruption into a one clock operation that is not. That is important if an ISR is modifying the same control word as main thread code.
*/
constexpr Address bandShift(Address byteOffset) {
  //5: 2^5 bits per 32 bit word. we expect a byte address here, so that one can pass values as read from the stm32 manual.
  return byteOffset << 5;
}

/** @return bitband address for given bit (default 0) of @param byte address.
this assumes that the byte address ends in 00, which all of the ones in the st manual do.
*/
constexpr Address bandFor(Address byteAddress, unsigned bitnum = 0) {
  //bit gets shifted by 2 as the underlying mechanism inspects the 32bit word address only, doesn't see the 2 lsbs of the address.
  //0xE000 0000: stm32 segments memory into 8 512M blocks, banding is within a block
  //0x0200 0000: indicates this is a bitband address
  //bit to lsbs of address |  byteaddress shifted up far enough for address space to go away | restore address space | bitband indicator.
  return ((bitnum << 2) | bandShift(byteAddress) | (byteAddress & 0xE0000000) | 0x02000000);
}

///** @return bitband address for given bit (default 0) of @param byte address.
//this assumes that the byte address ends in 00, which all of the ones in the st manual do.
//*/
//constexpr Address bandFor(Address byteAddress, unsigned bitnum = 0){
//  return bandit(byteAddress,bitnum);
//}

///** @return bitband address for given bit (default 0) of @param byte address.
//this assumes that the byte address ends in 00, which all of the ones in the st manual do.
//*/
//constexpr Address bandAddress(Address byteAddress, unsigned bitnum = 0){
//  return bandFor(byteAddress,bitnum);
//}

/** when you don't know the address at compile time use one of these, else use an SFRxxx. */
class ControlWord {
  volatile unsigned& item;

 public:
  ControlWord(Address dynaddr)
      : item(Ref(dynaddr)) {
    //#done
  }
  /** we often wish to return one of these, so we ensure the compiler knows it can do a bit copy or even a 'make in place'*/
  ControlWord(const ControlWord& other) = default;

  ControlWord(Address dynaddr, unsigned bitnum)
      : ControlWord(bandFor(dynaddr, bitnum)) {
    //#done
  }

  /** yes, const is allowed here! */
  void operator=(unsigned value) const {
    item = value;
  }

  /** mostly exists to appease compiler complaint about ambiguity of assignement. */
  void operator=(const ControlWord& other) const {
    item = other;
  }

  /** it is unproven if volatility is propagated through this wrapper. Check it for your compiler and flags. */
  operator unsigned() const {
    return item;
  }
};

/** Multiple contiguous bits in a register. Requires register to be R/W.
 * For write-only registers declare a union of int with struct of bitfields that describes the register. Manipulate an instance then assign it to an SFR8/16/32.
 * Note: This creates a class per sf register field, but the compiler should inline all uses making this a compile time burden but a runtime minimization.
 * Note: 'volatile' isn't used here as it is gratuitous when the variable isn't nominally read multiple times in a function.
 */
class ControlField {
  volatile unsigned& word;

  /** mask gets pre-positioned */
  const unsigned mask;
  const unsigned pos;

 public:
  ControlField(Address sfraddress, unsigned pos, unsigned width = 1)
      : word(Ref(sfraddress)), mask(bitMask(pos, width)), pos(pos) {}

 public:
  ControlField(const ControlField& other) = delete;
  ControlField() = delete;

  // read
  inline operator unsigned() const {
    return (word & mask) >> pos;  //the compiler should render this down to a bitfield extract instruction.
  }

  // write
  void operator=(unsigned value) const {
    word = ((value << pos) & mask) | (word & ~mask);  //the compiler should render this down to a bitfield insert instruction.
  }

  void operator+=(unsigned value) const {
    operator=(unsigned() + value);
  }
};

/** */
struct ControlBit : public BoolishRef {
  const volatile unsigned  bandAddress;

  ControlBit(Address sfraddress, unsigned bitnum):bandAddress(bandFor(sfraddress, bitnum)){}
  
  // read
  inline operator bool() const override {  
    return *(reinterpret_cast<volatile unsigned*>(bandAddress));
  }

  // write
  bool operator=(bool value) const override { 
    *(reinterpret_cast<unsigned*>(bandAddress)) = value;
    return value;
  }
};

/** a datum at a known absolute address */
template <typename Inttype, Address sfraddress>
struct SFRint {
  SFRint() = default;

  // read. If you assign this to an unsigned rather than Inttype you will incur a uxtb instruction.
  operator Inttype() const {
    return unsigned(Ref(sfraddress));
  }

  // write
  void operator=(unsigned value) const {
    Ref(sfraddress) = value;
  }
};

///** making all (conveniently predefined) SFR's unsigned presuming that all hardware values are unsigned. About the only exception is ADC values. */
//template<unsigned sfraddress> using SFR8  = SFRint<uint8_t, sfraddress>;
//template<unsigned sfraddress> using SFR16 = SFRint<uint16_t, sfraddress>;
//template<unsigned sfraddress> using SFR32 = SFRint<uint32_t, sfraddress>;

/** Multiple contiguous bits in a register. Requires register to be R/W.
 * For write-only registers declare a union of int with struct of bitfields that describes the register. Manipulate an instance then assign it to an SFR8/16/32.
 * Note: This creates a class per sf register field, but the compiler should inline all uses making this a compile time burden but a runtime minimalization.
 * Note: 'volatile' isn't used here as it is gratuitous when the variable isn't nominally read multiple times in a function.
 */
template <Address sfraddress, unsigned pos, unsigned width = 1>
class SFRfield {
  enum {
    /** mask positioned */
    mask = bitMask(pos, width)
  };

 public:
  SFRfield(const SFRfield& other) = delete;

  SFRfield() = default;  //this constructor is needed due to use of explicit on the other constructor

  /** this constructor is intended to be used for setting a value into a register which has no reference other than the assignment, but it is not easy to debug its use when something goes horribly wrong. */
  explicit SFRfield(unsigned initlizer) {
    this->operator=(initlizer);
  }

  // read
  inline operator unsigned() const {  // NOLINT
    return (Ref(sfraddress) & mask) >> pos;
  }

  // write
  inline void operator=(unsigned value) const {  // NOLINT
    Ref(sfraddress) = ((value << pos) & mask) | (Ref(sfraddress) & ~mask);
  }

  void operator+=(unsigned value) const {
    Ref(sfraddress) = Ref(sfraddress) + value;
  }
};

/** single bit, ignoring the possibility it is in bitbanded memory.
 *  This is NOT derived from SFRfield as we can do some optimizations that the compiler might miss (or developer might have disabled)*/
template <Address sfraddress, unsigned pos>
class SFRbit : public BoolishRef {
  enum {
    mask = bitMask(pos)
  };

  inline SFR& sfr() const {
    return *reinterpret_cast<SFR*>(sfraddress);
  }

 public:
  SFRbit() = default;

  // read
  inline operator bool() const override {  // NOLINT
    return (sfr() & mask) != 0;
  }

  // write
  bool operator=(bool value) const override {  // NOLINT
    if (value) {
      sfr() |= mask;
    } else {
      sfr() &= ~mask;
    }
    return value;
  }
};


/** if your bit is in bitband space use this in stead of SFRbit */
template <Address sfraddress, unsigned bitnum>
struct SFRbandbit: public BoolishRef {
  enum {
    bandAddress = bandFor(sfraddress, bitnum),
  };
  // read
  inline operator bool() const override {  // NOLINT
    return *(reinterpret_cast<volatile unsigned*>(bandAddress));
  }

  // write
  bool operator=(bool value) const override {  // NOLINT
    *(reinterpret_cast<unsigned*>(bandAddress)) = value;
    return value;
  }
};