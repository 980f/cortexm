#pragma once
#include "eztypes.h"

/* stm family common stuff */
#include "peripheraltypes.h" //stm32 specific peripheral construction aids.

/** peripheral base addresses are computable from their indexes into the clock control registers: */
constexpr Address APB_Block(unsigned bus2, unsigned slot) { return (PeripheralBase | bus2 << 16 | slot << 10); }

constexpr Address APB_Band(unsigned bus2, unsigned slot) { return (PeripheralBand | bus2 << 21 | slot << 15); }

const Address RCCBASE(0x40021000);

/** each peripheral's reset pin, clock enable, and bus address are computable from 2 simple numbers.
working our way up to a template. */
struct APBdevice {
  const u8 bus2; //boolean, packed
  const u8 slot; //max 32 items (value is 0..31)
  /** base device address @see registerAddress() for multi-bit control */
  const Address blockAddress;
  /** base bit band address. @see bit() to access a single bit control */
  const Address bandAddress;
  /** base used for calculating this device's bits in RCC device. */
  const Address rccBitter;

protected:
  /** @return bit address given the register address of the apb2 group*/
  unsigned rccBit(unsigned basereg) const {
    return bandFor(rccBitter | bandShift(basereg));
  }
  /** this class is cheap enough to allow copies, but why should we?: because derived classes sometimes want to be copied eg Port into pin).*/
  APBdevice(const APBdevice &other) = default;

public:
  /** @param bus and slot are per st documentation, hence a minus 1.*/
  APBdevice(unsigned int stbus, unsigned int slot);
  /** activate and release the module reset */
  void reset(void) const;
  /** control the clock, off reduces chip power */
  void setClockEnable(bool on = true) const;
  /** @returns whether clock is on */
  bool isEnabled(void) const;
  /** reset and enable clock */
  void init(void) const;
  /** get base clock for this module */
  u32 getClockRate(void) const;
  /** @returns address of a register, @param offset is value from st's manual (byte address) */
  Address registerAddress(unsigned offset) const {
    return blockAddress + offset;
  }
  /** @returns bit band address of bit of a register, @param offset is value from st's manual (byte address) */
  ControlWord bit(Address offset, unsigned bit) const {
    return ControlWord(bandAddress + bandFor(offset, bit));
  }
};

template <unsigned stbus, unsigned slot>
struct APBperiph {
  enum {
    bus2 = stbus - 1,
    //  const u8 slot; //max 32 items (value is 0..31)
    /** base device address @see registerAddress() for multi-bit control */
    blockAddress = APB_Block(bus2, slot),
    /** base bit band address. @see bit() to access a single bit control */
    bandAddress = APB_Band(bus2, slot),
    /** base used for calculating this device's bits in RCC device. */
    rccBitat = bandFor(RCCBASE | ((1 - bus2) << 2), slot),
  };
  /** @return bit address given the register address of the apb2 group*/
  constexpr unsigned rccBit(unsigned basereg) const {
    return bandFor(rccBitat | bandShift(basereg));
  }

  constexpr unsigned rccWord(unsigned basereg) const {
    return RCCBASE | ((1 - bus2) << 2) + basereg;
  }

  const Controlbit<rccWord(0x0C), slot> resetter;
  const Controlbit<rccWord(0x18), slot> clocker;

  /** reset and enable clock */
  void init(void) const {
    resetter = 1;
    resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
    clocker = 1;
  }

  /** activate and release the module reset */
  void reset(void) const {
    //  ControlWord resetter(rccBit(0x0C));
    resetter = 1;
    resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
  };

  
  /** control the clock, off reduces chip power */
  void setClockEnable(bool on = true) const {
    //   ControlWord clocker(rccBit(0x18));
    clocker = on;
  }

  /** @returns whether clock is on */
  bool isEnabled(void) const {
    return clocker;
  }

  /** get base clock for this module */
  u32 getClockRate(void) const {
    return clockRate(bus2 + 1);
  }
};

