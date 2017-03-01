#pragma once

#include "eztypes.h"
#include "bitbanger.h"
#include "boolish.h"

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
#define soliton(type, address) type & the ## type = *reinterpret_cast <type *> (address);

/** the following are mostly markers, but often it is tedious to insert the 'volatile' and dangerous to leave it out. */
typedef volatile u32 SFR;
/** marker for non-occupied memory location */
typedef const u32 SKIPPED;
// packable byte:
//typedef volatile u8 SFR8;
//typedef volatile u16 SFR16;
// todo: 16 bit ones, with word spacers.
// todo: macro for skip block


/** most cortex devices follow arm's suggestion of using this block for peripherals */
const u32 PeripheralBase(0x40000000);//1<<30

//GPIO interrupt configuration options. Some devices may not support some options, but most do so this is defined here.
enum IrqStyle {
  NotAnInterrupt = 0, // in case someone forgets to explicitly select a mode
  AnyEdge, // edge, either edge, input mode buslatch
  LowActive, // level, pulled up
  HighActive, // level, pulled down
  LowEdge, // edge, pulled up
  HighEdge   // edge, pulled down
};

/** a datum at a known absolute address */
template <typename Inttype, unsigned sfraddress> struct SFRint {
  // read
  operator unsigned() const {
    return unsigned(*reinterpret_cast<volatile Inttype*>(sfraddress));
  }

  // write
  void operator =(unsigned value) const {
    *reinterpret_cast<volatile Inttype*>(sfraddress)=value;
  }

};

/** making all (conveniently predefined) SFR's unsigned presuming that all hardware values are unsigned. About the only exception is ADC values. */
template <unsigned sfraddress> using SFR8  = SFRint<uint8_t, sfraddress>;
template <unsigned sfraddress> using SFR16 = SFRint<uint16_t,sfraddress>;
template <unsigned sfraddress> using SFR32 = SFRint<uint32_t,sfraddress>;


/** Multiple contiguous bits in a register. Requires register to be R/W.
 * For write-only registers declare a union of int with struct of bitfields that describes the register. Manipulate an instance then assign it to an SFR8/16/32.
 * Note: This creates a class per sf register field, but the compiler should inline all uses making this a compile time burden but a runtime minimalization.
 * Note: 'volatile' isn't used here as it is gratuitous when the variable isn't nominally read multiple times in a function.
 */
template <unsigned sfraddress, int pos, int width = 1> class SFRfield {
  enum {
    /** mask positioned */
    mask = bitMask(pos,width)
  };

  inline SFR &sfr() const {
    return *reinterpret_cast<SFR *>(sfraddress);
  }
private:
  SFRfield(const SFRfield &other) = delete;

public:
  SFRfield(){
    //this constructor is needed due to use of explicit on the other constructor
  }

  /** this constructor is intended to be used for setting a value into a register which has no reference other than the assignment, but it is not easy to debug its use when something goes horribly wrong. */
  explicit SFRfield(unsigned initlizer){
    this->operator =(initlizer);
  }

  // read
  inline operator uint32_t() const {
    return (sfr() & mask) >> pos;
  }

  // write
  inline void operator =(uint32_t value) const {
    sfr() = ((value << pos) & mask) | (sfr() & ~mask);
  }

  void operator +=(unsigned value) const {
    sfr()=sfr()+value;
  }
};


/** single bit, ignoring the possibility it is in bitbanded memory.
 *  This is NOT derived from SFRfield as we can do some optimizations that the compiler might miss (or developer might have disabled)*/
template <unsigned sfraddress, int pos> class SFRbit : public BoolishRef {
  enum {
    mask = bitMask(pos)
  };

  inline SFR &sfr() const {
    return *reinterpret_cast<SFR *>(sfraddress);
  }
public:
  // read
  inline operator bool() const {
    return (sfr() & mask) != 0;
  }
  // write
  bool operator =(bool value) const{
    if(value) {
      sfr() |= mask;
    } else {
      sfr() &= ~mask;
    }
    return value;
  }
};
