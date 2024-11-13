#pragma once


#pragma clang diagnostic push
//we ignore the following warnings as this file exists to make hardware registers appear to be simple variables
#pragma ide diagnostic ignored "google-explicit-constructor"
#pragma ide diagnostic ignored "misc-unconventional-assign-operator"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "eztypes.h"
#include "bitbasher.h"
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
I am working on replacing *'s with &'s, its a statistical thing herein as to which is better.

 #pragma ide diagnostic ignored "google-explicit-constructor"
 #pragma ide diagnostic ignored "misc-unconventional-assign-operator"

 we want these classes to appear to be normal program variables, so that we can replace instances of them with normal program variables to disable them with only changing the declaration.

*/
/** for a private single instance block */
#define soliton(type, address) type& the##type = *reinterpret_cast<type*>(address);

/** marker for an address, will eventually feed into a *reinterpret_cast<unsigned *>() */
using Address = unsigned;//address space of this device.

/* an attempt to suppress warnings about integer to pointer casts, while still leaving that warning on to catch unintentional ones */
union AddressCaster {
  unsigned number;
  void *pointer;
};

/* this function exists to hide some verbose casting */
template<typename Scalar> constexpr Scalar &Ref(Address address) {
  AddressCaster pun {address};
  return *static_cast<Scalar *>(pun.pointer);
}



/** A 32 bit item at a known address.
 * when you don't know the address at compile time use one of these, else use an SFRxxx.
 * This class essentially wraps the Ref<> template with operator overloads */
class ControlWord {

protected:
  volatile unsigned &item;
public:
  explicit constexpr ControlWord(Address dynaddr)
    : item(Ref<unsigned>(dynaddr)) {
    //#done
  }

  /** we often wish to return one of these, so we ensure the compiler knows it can do a bit copy or even a 'make in place'*/
  constexpr ControlWord(const ControlWord &other) = default;

//use ControlBit instead. This one allows the ambiguity of writing a value other than 1 or 0 to a bit band item.
//  constexpr ControlWord(Address dynaddr, unsigned bitnum)
//    : ControlWord(bandFor(dynaddr, bitnum)) {
//    //#done
//  }

  void operator=(unsigned value) const ISRISH {
    item = value;
  }


  void operator|=(unsigned value) const ISRISH {
    item |= value;
  }

  void operator&=(unsigned value) const ISRISH {
    item &= value;
  }

  /** mostly exists to appease compiler complaint about ambiguity of assignment. */
  void operator=(const ControlWord &other) const ISRISH {
    item = other.item;
  }


//we do want implicit conversions here, the goal of the class is to make accessing a control word look syntactically like accessing a normal variable.
  /** it is unproven if volatility is propagated through this wrapper. Check it for your compiler and flags. */
  operator unsigned() const ISRISH {
    return item;
  }
};

template<class Mustbe32> struct ControlStruct {

protected:
  volatile unsigned &item;
public:
  explicit constexpr ControlStruct(Address dynaddr)
    : item(Ref<unsigned>(dynaddr)) {
    //#done
  }

  /** we often wish to return one of these, so we ensure the compiler knows it can do a bit copy or even a 'make in place'*/
  constexpr ControlStruct(const ControlStruct &other) = default;

  void operator=(const Mustbe32 &value) const ISRISH {
    item = *reinterpret_cast<const unsigned *>(&value);
  }

  void operator=(Mustbe32 &&value) const ISRISH {
    item = *reinterpret_cast<const unsigned *>(&value);
  }

  constexpr  Mustbe32 operator()() const ISRISH {
    unsigned read = item;
    return *reinterpret_cast<const Mustbe32 *>(&read);
  }
};

/** when you don't know the address at compile time use one of these, else use an SFRxxx.
 * This is for fields which are byte aligned and some multiple of 8 bits */
template<typename IntType>
class ControlItem {

protected:
  volatile IntType &item;
public:
  explicit constexpr ControlItem(Address dynaddr)
    : item(Ref<IntType>(dynaddr)) {
    //#done
  }

  /** we often wish to return one of these, so we ensure the compiler knows it can do a bit copy or even a 'make in place'*/
  constexpr ControlItem(const ControlItem &other) = default;

//using void return as we don't want to trust what the compiler might do with the 'volatile'
  void operator=(IntType value) const ISRISH {
    item = value;
  }

  void operator|=(IntType value) const ISRISH {
    item |= value;
  }

  void operator&=(IntType value) const ISRISH {
    item &= value;
  }

  /** mostly exists to appease compiler complaint about ambiguity of assignment. */
  void operator=(const ControlItem &other) const ISRISH {
    item = other.item;
  }

//we do want implicit conversions here, the goal of the class is to make accessing a control word look syntactically like accessing a normal variable.
  /** it is unproven if volatility is propagated through this wrapper. Check it for your compiler and flags. */
  operator IntType() const ISRISH {
    return item;
  }
};

/** Multiple contiguous bits in a register. Requires register to be R/W.
 * For write-only registers declare a union of int with struct of bitfields that describes the register. Manipulate an instance then assign it to an SFR8/16/32.
 * Note: This creates a class per sf register field, but the compiler should inline all uses making this a compile time burden but a runtime minimization.
 * Note: 'volatile' isn't used here as it is gratuitous when the variable isn't nominally read multiple times in a function.
 */
class ControlField {
  volatile unsigned &word;

  /** mask gets pre-positioned */
  const unsigned mask;
  const unsigned pos;

public:
  constexpr ControlField(Address sfraddress, unsigned pos, unsigned width)
    : word(Ref<unsigned>(sfraddress)), mask(bitMask(pos, width)), pos(pos) {
  }

public:
  //this is an immutable object, so copying should be just fine, and is needed for move behavior which is needed by object factories.
  // constexpr ControlField(const ControlField &other) = delete;


  ControlField() = delete; //fail a compile if no args are given.

  // read
  operator unsigned() const {
    return (word & mask) >> pos;  //the compiler should render this down to a bitfield extract instruction.
  }

  /** assign, which for hardware registers might not result in a value equal to the @param value given, @returns the ACTUAL value */
  unsigned operator=(unsigned value) const {
    word = ((value << pos) & mask) | (word & ~mask);  //the compiler should render this down to a bitfield insert instruction.
    return operator unsigned();
  }

  /** increment seems unlikely, someone add a use case else we might make this go away. */
  unsigned operator+=(unsigned value) const {
    operator=(unsigned() + value);
    return operator unsigned();
  }
  //add more operators as need arises
};

/** single bit, ignoring the possibility it is in bitbanded memory.
 *  This is NOT derived from ControlField as we can do some optimizations that the compiler might miss (or developer might have disabled) */

class ControlBool : public BoolishRef {
  volatile unsigned &word;
  /** mask gets pre-positioned */
  const unsigned mask;
  const unsigned pos;

public:
  constexpr ControlBool(Address sfraddress, unsigned pos)
    : word(Ref<unsigned>(sfraddress)), mask(bitMask(pos, 1)), pos(pos) {
  }

public:
  constexpr ControlBool(const ControlField &other) = delete;

  ControlBool() = delete;

  // read
  operator bool() const override {
    return word & mask;
  }

  /** assign, which for hardware registers might not result in a value equal to the @param value given, @returns the ACTUAL value */
  bool operator=(bool value) const override {
    if (value) {
      word |= mask;
    } else {
      word &= ~mask;
    }
    return operator bool();
  }

};


/** a datum at a known absolute address */
template<typename Inttype, Address sfraddress>
struct SFRint {
  constexpr SFRint() = default;

  // read.
  operator Inttype() const {
    return Ref<Inttype>(sfraddress);
  }

  // write
  void operator=(Inttype value) const {
    Ref<Inttype>(sfraddress) = value;
  }
};

///** making all (conveniently predefined) SFR's unsigned presuming that all hardware values are unsigned. About the only exception is ADC values. */
template<unsigned sfraddress> using SFR8  = SFRint<uint8_t, sfraddress>;
template<unsigned sfraddress> using SFR16 = SFRint<uint16_t, sfraddress>;
template<unsigned sfraddress> using SFR32 = SFRint<uint32_t, sfraddress>;
//legacy
using SFR=volatile unsigned;


/** Multiple contiguous bits in a register. Requires register to be R/W.
 * For write-only registers declare a union of int with struct of bitfields that describes the register. Manipulate an instance then assign it to an SFR8/16/32.
 * Note: This creates a class per sf register field, but the compiler should inline all uses making this a compile time burden but a runtime minimalization.
 * Note: 'volatile' isn't used here as it is gratuitous when the variable isn't nominally read multiple times in a function.
 */
template<Address sfraddress, unsigned pos, unsigned width = 1>
class SFRfield {
  enum {
    /** mask positioned */
    mask = bitMask(pos, width)
  };

public:
  SFRfield(const SFRfield &other) = delete;

  constexpr SFRfield() = default;  //this constructor is needed due to use of explicit on the other constructor

  /** this constructor is intended to be used for setting a value into a register which has no reference other than the assignment, but it is not easy to debug its use when something goes horribly wrong. */
  explicit SFRfield(unsigned initlizer) {
    this->operator=(initlizer);
  }

  // read
  operator unsigned() const {
    return (Ref<unsigned>(sfraddress) & mask) >> pos;
  }

  // write
  void operator=(unsigned value) const {
    Ref<unsigned>(sfraddress) = ((value << pos) & mask) | (Ref<unsigned>(sfraddress) & ~mask);
  }

  void operator+=(unsigned value) const {
    operator=(operator unsigned() + value);//todo:M optimize if compiler doesn't remove the shifts of all but this function's value argument.
  }
};

/** single bit, ignoring the possibility it is in bitbanded memory.
 *  This is NOT derived from SFRfield as we can do some optimizations that the compiler might miss (or developer might have disabled)*/
template<unsigned sfraddress, unsigned pos>
class SFRbit : public BoolishRef {
  enum {
    mask = bitMask(pos)
  };
public:
  constexpr SFRbit() = default;

  // read
  operator bool() const override {
    return (Ref<unsigned>(sfraddress) & mask) != 0;
  }

  // write
  bool operator=(bool value) const override {
    if (value) {
      Ref<unsigned>(sfraddress) |= mask;
    } else {
      Ref<unsigned>(sfraddress) &= ~mask;
    }
    return value;
  }
};


/** most cortex devices follow arm's suggestion of using this block for peripherals */
const Address PeripheralBase {0x4000'0000};  //1<<30


//cortexM 'private peripherals'
// the SCB is kinda like a peripheral, but we may just inline this at each point of use. The manual lists both absolute and relative addresses.
constexpr Address SCB(unsigned offset) {
  return 0xE000'ED00u + offset;
}

#pragma clang diagnostic pop
