#pragma once

#include "eztypes.h"

/* stm family common stuff */
#include "peripheraltypes.h" //stm32 specific peripheral construction aids.
//#include "clocks.h" //for clock data type

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"

#if DEVICE == 103
enum BusNumber: u8 {//#this enum is used for RCC register addressing
  CPU,
  AHB1, //even though there is but 1 adding the '1' saves some #ifdef'ing in clock related code.
  APB0, //marker, not actually a valid value, used for getting which apb a BusNumber is.
  APB1, APB2
  ,ADCbase
};

const Address RCCBASE(0x40021000U);//0th offset.
const unsigned resetOffset=0x0C;
const unsigned clockOffset=0x18;

#elif DEVICE == 407
enum BusNumber: u8 {//#this enum is used for RCC register addressing
  CPU ,
  AHB1, AHB2, AHB3, //3 buses which have some APDevice like characteristics
  APB0, //for calculations, there is no bus named 'APB0'
  APB1 = 5, APB2 //2 actual APB buses.
};

const Address RCCBASE(0x40023800U);//0th offset.
const unsigned resetOffset = 0x10;
const unsigned clockOffset = 0x30;
//todo: low power mode.
#endif

//type for clock setting.
using Hertz=unsigned;

/** peripheral base addresses are computable from their indexes into the clock control registers: */
constexpr Address APB_Block(BusNumber bus2, unsigned slot) { return (PeripheralBase | (bus2 - APB0) << 16u | slot << 10u); }

constexpr Address APB_Band(BusNumber bus2, unsigned slot) { return (PeripheralBand | (bus2 - APB0) << 21u | slot << 15u); }

///** index used for rcc bit offset calculations */
//constexpr unsigned rccBus(unsigned stnum, bool ahb) {
//#if DEVICE == 103
//  //ahb not yet supported, each device copies code from this module.
//    return 1-stnum; //bus 2 is the first of a pair of which bus1 is the second. ST doesn't like consistency.
//#elif DEVICE == 407
//  return ahb ? stnum - 1 : stnum + 3;//3 ahb's,skip one,followed by 2 apb's
//#endif
//}

/** each APB peripheral's reset pin, clock enable, and bus address are computable from 2 simple numbers.
some AHB devices are very similar, to the point where we use a variant constructor rather than have a separate class.
 */
struct APBdevice {
  const BusNumber rbus; //internal index
  const u8 slot; //max 32 items (value is 0..31)
  /** base device address @see registerAddress() for multi-bit control */
  const Address blockAddress;
  /** base bit band address. @see bit() to access a single bit control */
  const Address bandAddress;
  /** base used for calculating this device's bits in RCC device. */
  const Address rccBitter;

protected:
  /** @return bit address given the register address of the apb2 group*/
  Address rccBit(Address basereg) const {
    return rccBitter + bandShift(basereg);
  }
  /** this class is cheap enough to allow copies, but why should we?: because derived classes sometimes want to be copied eg Port into pin).*/
  constexpr APBdevice(const APBdevice &other) = default;

public:
  /** Actual APB devices  @param bus and slot are per st documentation */
  constexpr APBdevice(BusNumber stbus, unsigned slot) :
    rbus(stbus),
    slot(slot),
    blockAddress(APB_Block(rbus, slot)),
    bandAddress(APB_Band(rbus, slot)),
    rccBitter(bandFor(RCCBASE | (rbus << 2u), slot)) {}

/** AHB devices  @param bus and slot are per st documentation */
  constexpr APBdevice(BusNumber stbus, unsigned slot, unsigned rawAddress) :
    rbus(stbus),
    slot(slot),
    blockAddress(rawAddress),
    bandAddress(bandFor(rawAddress, slot)),
    rccBitter(bandFor(RCCBASE | (rbus << 2u), slot)) {}

  /** activate and release the module reset */
  void reset() const;
  /** control the clock, off reduces chip power */
  void setClockEnable(bool on = true) const;
  /** @returns whether clock is on */
  bool isEnabled() const;
  /** reset and enable clock */
  void init() const;
  /** get base clock for this module */
  Hertz getClockRate() const;
  /** @returns address of a register, @param offset is value from st's manual (byte address) */
  constexpr Address registerAddress(unsigned offset) const {
    return blockAddress + offset;
  }
  /** @returns bit band address of bit of a register, @param offset is value from st's manual (byte address) */
  constexpr ControlWord bit(Address offset, unsigned bit) const {
    return ControlWord(bandFor(blockAddress + offset, bit));//bandAddress and bandFor were both setting the 0200 0000 bit.
  }

  /** @returns reference to a word member of the hardware object, @param offset if value from st's manual (byte address) */
  constexpr ControlWord word(Address offset) const {
    return ControlWord(blockAddress + offset);
  }
};

/** for items which only have a single instance, or for which the luno is a compile time constant and you need speed over code space use this instead of APBdevice.*/
template<BusNumber stbus, unsigned slot>
struct APBperiph {
  enum {
    rbus = stbus,
    /** base device address @see registerAddress() for multi-bit control */
    blockAddress = APB_Block(rbus, slot),
    /** base bit band address. @see bit() to access a single bit control */
    bandAddress = APB_Band(rbus, slot),
    /** base used for calculating this device's bits in RCC device. */
    rccBitat = bandFor(RCCBASE | (rbus << 2), slot),
  };
  /** @return bit address given the register address of the apb2 group*/
  constexpr static Address rccBit(unsigned basereg) {
    return bandFor(rccBitat | bandShift(basereg));
  }

  constexpr static Address rccWord(unsigned basereg) {
    return RCCBASE | (rbus << 2) + basereg;
  }

  //creates compile time complaints versus its lack creating IDE warnings __unused  //write only hardware field.
  const SFRbandbit<rccWord(resetOffset), slot> resetter;
  const SFRbandbit<rccWord(clockOffset), slot> clocker;
#pragma clang diagnostic pop

  /** reset and enable clock */
  void init() const {
    resetter = 1;
    resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
    clocker = 1;
  }

  /** activate and release the module reset */
  void reset() const {
    resetter = 1;
    resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
  };

  /** control the clock, off reduces chip power */
  void setClockEnable(bool on = true) const {
    clocker = on;
  }

  /** @returns whether clock is on */
  bool isEnabled() const {
    return clocker;
  }

  /** get base clock for this module */
  u32 getClockRate() const {
    return clockRate(rbus + 1);//todo: F407 inside clockRate
  }
};


