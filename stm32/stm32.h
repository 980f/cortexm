#pragma once

#pragma clang diagnostic push
//this is a library, pieces will often be unused.
#pragma ide diagnostic ignored "modernize-use-nodiscard"



/* stm family common stuff */

#ifndef DEVICE
#error "You must define DEVICE to one of the known ones, such as: 103,407,411,452"
#endif

#include "peripheraltypes.h" //stm32 specific peripheral construction aids.
using namespace CortexM;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"

/* The BusNumber enum is used to compute control bit addresses for RCC functions. */
#if DEVICE == 103
#include "peripheralband.h"

enum BusNumber: uint8_t { //#this enum is used for RCC register addressing, its numerical value matters very much.
  CPU, AHB1 //even though there is but 1 AHB adding the '1' to its name saves some #ifdef'ing in clock related code with other chips
  , APB1 = 3, APB2, ADCbase 
};

constexpr int RCCoffset(BusNumber bus){
  return (bus==AHB1)?-1:(bus==APB1)?1:0;
}

constexpr Address RCCBASE(0x4002'1000); //0th offset.
constexpr Address FLASHBASE(0x4002'2000);

const unsigned resetOffset = 0x0C;
const unsigned clockOffset = 0x18;

#elif DEVICE == 407 || DEVICE==411
#include "peripheralband.h"

enum BusNumber: uint8_t {//#this enum is used for RCC register addressing
  CPU //used for clock rate function index
  ,AHB1=1, AHB2, AHB3 //3 buses which have some APDevice like characteristics
  //there is a gap in values here, this enum is used arithmetically
  ,APB1 = 5, APB2
};

constexpr int RCCoffset(BusNumber bus){
  return (bus-AHB1);
}

const Address RCCBASE(0x4002'3800);//0th offset.
constexpr Address FLASHBASE(0x4002'3C00);
constexpr Address GPIOBASE(0x4002'0000);

const unsigned resetOffset = 0x10;
const unsigned clockOffset = 0x30;
const unsigned lpClockOffset = 0x50;//clock enables for when in low power mode


#elif DEVICE == 452
#include "peripheralband.h"

enum BusNumber : uint8_t {//#this enum is used for RCC register addressing
  CPU //used for clock rate function index
  , AHB1=1, AHB2, AHB3
  , APB1 = 5
  //there are more than 32 periphs in this device! so we leave a gap here.
  , APB2 = 7 //2 actual APB buses.
};

constexpr Address RCCBASE(0x4002'1000);
constexpr Address FLASHBASE(0x4002'2000);
constexpr Address GPIOBASE(0x4800'0000);

constexpr unsigned resetOffset = 0x28;
constexpr unsigned clockOffset = 0x48;
#else
#error "Your DEVICE define is missing or that device is not yet coded for"
#endif



//type for clock setting.
using Hertz = unsigned;

/** peripheral base addresses are computable from their indexes into the clock control registers: */
constexpr Address APB_Block(BusNumber bus2, unsigned slot) {
  return (PeripheralBase | (bus2 - APB1) << 16u | slot << 10u);
}

constexpr Address APB_Band(BusNumber bus2, unsigned slot) {
  return bandFor(APB_Block(bus2, slot), 0);
}


/** each APB peripheral's reset pin, clock enable, and bus address are computable from 2 simple numbers.
some AHB devices are very similar, to the point where we use a variant constructor rather than have a separate class.
 */
struct APBdevice {
  const BusNumber rbus; //internal index
  const uint8_t slot; //max 32 items (value is 0..31)

  /** base device address @see registerAddress() for multi-bit control */
  const Address blockAddress;
  /** base bit band address. @see bit() to access a single bit control */
  const Address bandAddress;

protected:
  /** @return bit address given the register address of the apb2 group*/
  constexpr Address rccBit(Address basereg) const {
    return bandFor(RCCBASE + basereg+(RCCoffset(rbus) << 2), slot);
  }

  /** this class is cheap enough to allow copies, but why should we?: because derived classes sometimes want to be copied eg Port into pin).*/
  constexpr APBdevice(const APBdevice &other) = default;

public:
  /** Actual APB devices  @param bus and slot are per st documentation */
  
  /** AHB devices are almost the same as APB devices,  @param bus and slot are per st documentation, but you must provide the base address explicitly */
  constexpr APBdevice(BusNumber stbus, unsigned slot, unsigned rawAddress) : rbus(stbus) 
    , slot(slot) 
    , blockAddress(rawAddress) 
    , bandAddress(bandFor(rawAddress, slot)) 
     {}


  constexpr APBdevice(BusNumber stbus, unsigned slot) : APBdevice(stbus,slot,APB_Block(stbus, slot)){}

  /** activate and release the module reset */
  void reset() const {
    ControlWord resetter(rccBit(resetOffset));
    resetter = 1;
    resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
  }

  /** control the clock, off reduces chip power */
  void setClockEnable(bool on = true) const {
    ControlWord(rccBit(clockOffset)) = on;
  }

  //todo: if supports LP clock system (finish) add in low power clock enable, to either here or an added arg to setClockEnable.

  /** @returns whether clock is on */
  bool isEnabled() const {
    //  (reset on forces the clock off (I think) so we only have to check one bit)
    return ControlWord(rccBit(clockOffset));
  }

  /** reset and enable clock */
  void init() const { //frequently used combination of reset then enable clock.
    reset();
    setClockEnable();
  }

  /** conditionally reset and enable */
  void beEnabled() const {
    if (!isEnabled()) {
      init();
    }
  }

  /** get base clock for this module */
  Hertz getClockRate() const;

  /** @returns address of a register, @param offset is value from st's manual (byte address) */
  constexpr Address registerAddress(unsigned offset) const {
    return blockAddress + offset;
  }

  /** @returns bit band address of bit of a register, @param offset is value from st's manual (byte address) */
  constexpr ControlWord bit(Address offset, unsigned bit) const {
    return ControlWord(bandFor(blockAddress + offset, bit)); //bandAddress and bandFor were both setting the 0200 0000 bit.
  }

  constexpr ControlField field(Address offset, unsigned pos, unsigned width) const {
    return ControlField(registerAddress(offset), pos, width);
  }

  /** @returns reference to a word member of the hardware object, @param offset if value from st's manual (byte address) */
  constexpr ControlWord word(Address offset) const {
    return ControlWord(blockAddress + offset);
  }
};

/** for items which only have a single instance, or for which the luno is a compile time constant and you need speed over code space use this instead of APBdevice.*/
template<BusNumber stbus, unsigned slot> struct APBperiph {
  enum {
    rbus = stbus //
    /** base device address @see registerAddress() for multi-bit control */
    , blockAddress = APB_Block(rbus, slot) //
    /** base bit band address. @see bit() to access a single bit control */
    , bandAddress = APB_Band(rbus, slot) 
  };


  constexpr static Address rccWord(unsigned basereg) {
    return RCCBASE + basereg+(RCCoffset(rbus) << 2);
  }

  //creates compile time complaints versus its lack creating IDE warnings __unused  //write only hardware field.
  const SFRbandbit<rccWord(resetOffset), slot> resetter;
  const SFRbandbit<rccWord(clockOffset), slot> clocker;
#pragma clang diagnostic pop


  /** activate and release the module reset */
  void reset() const {
    resetter = 1;
    resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
  };

  /** control the clock, off reduces chip power */
  void setClockEnable(bool on = true) const {
    clocker = on;
  }

  /** reset and enable clock */
  void init() const {
    reset();
    clocker = 1;
  }

  /** @returns whether clock is on */
  bool isEnabled() const {
    return clocker;
  }

  /** get base clock for this module */
  uint32_t getClockRate() const {
    return clockRate(rbus);
  }
};

#pragma clang diagnostic pop
