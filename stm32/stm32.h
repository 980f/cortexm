#pragma once

#pragma clang diagnostic push
//this is a library, pieces will often be unused.
#pragma ide diagnostic ignored "modernize-use-nodiscard"

/* stm family common stuff */

#ifndef DEVICE
#error "You must define DEVICE to one of the known ones, 103,407,452"
#endif

#include "peripheraltypes.h" //stm32 specific peripheral construction aids.

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"

/* The BusNumber enum is used to compute control bit addresses for RCC functions. */
#if DEVICE == 103
enum BusNumber: uint8_t {//#this enum is used for RCC register addressing
  CPU
  ,AHB1 //even though there is but 1 AHB adding the '1' to its name saves some #ifdef'ing in clock related code with other chips
  ,APB1=3, APB2
  ,ADCbase
};

constexpr Address RCCBASE(0x4002'1000);//0th offset.
constexpr Address FLASHBASE(0x4002'2000);
constexpr Address GPIOBASE(0x4001'0800);//+0400 for each letter.

const unsigned resetOffset=0x0C;
const unsigned clockOffset=0x18;

#elif DEVICE == 407
enum BusNumber: uint8_t {//#this enum is used for RCC register addressing
  CPU //used for clock rate function index
  ,AHB1=1, AHB2, AHB3 //3 buses which have some APDevice like characteristics
  //there is a gap here
  ,APB1 = 5, APB2
};

const Address RCCBASE(0x4002'3800U);//0th offset.
constexpr Address FLASHBASE(0x4002'3C00u);
constexpr Address GPIOBASE(0x4002'0000u);

const unsigned resetOffset = 0x10;
const unsigned clockOffset = 0x30;

#elif DEVICE == 452
enum BusNumber : uint8_t {//#this enum is used for RCC register addressing
  CPU //used for clock rate function index
  , AHB1=1, AHB2, AHB3
  , APB1 = 5
  //there are more than 32 periphs in this device! so we leave a gap here.
  , APB2 = 7 //2 actual APB buses.
};

constexpr Address RCCBASE(0x4002'1000u);
constexpr Address FLASHBASE(0x4002'2000u);
constexpr Address GPIOBASE(0x4800'0000u);

constexpr unsigned resetOffset = 0x28;
constexpr unsigned clockOffset = 0x48;
#endif

//todo:M move much of the rest into this namespace
namespace stm32 {
  constexpr bool isRam(AddressCaster address) {
    return address.number>>29==2>>1;//we exclude CCM at 0x1000 as it is not accessible to DMA and no-one else should care about "is ram"
  }

  constexpr bool isRom(AddressCaster address){
    return address.number>>29==0;
  }

  constexpr bool isPeripheral(AddressCaster address){
    return address.number>>29 == 4>>1 || address.number>>29 == 0xE>>1;//the second is cortex core peripherals
  }

  constexpr bool isBandable(AddressCaster address){
    return (address.number>>29 == 4>>1 || address.number>>29 == 2>>1) && (((address.number&~BandGroup)>>20) ==0);
  }
}

//type for clock setting.
using Hertz = unsigned;

/** peripheral base addresses are computable from their indexes into the clock control registers: */
constexpr Address
APB_Block(BusNumber bus2, unsigned slot) { return (PeripheralBase | (bus2 - APB1) << 16u | slot << 10u); }

constexpr Address
APB_Band(BusNumber bus2, unsigned slot) { return (PeripheralBand | (bus2 - APB1) << 21u | slot << 15u); }

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
  constexpr Address rccBit(Address basereg) const {
    return rccBitter + bandShift(basereg);
  }

  /** this class is cheap enough to allow copies, but why should we?: because derived classes sometimes want to be copied eg Port into pin).*/
  constexpr APBdevice(const APBdevice &other) = default;

public:
  /** Actual APB devices  @param bus and slot are per st documentation */
  constexpr APBdevice(BusNumber stbus, unsigned slot) :
    rbus(stbus)   //
    , slot(slot)  //
    , blockAddress(APB_Block(rbus, slot)) //
    , bandAddress(APB_Band(rbus, slot))   //
    , rccBitter(bandFor(RCCBASE | ((rbus - AHB1) << 2u), slot)) {}

/** AHB devices  @param bus and slot are per st documentation */
  constexpr APBdevice(BusNumber stbus, unsigned slot, unsigned rawAddress) :
    rbus(stbus)   //
    , slot(slot)  //
    , blockAddress(rawAddress)   //
    , bandAddress(bandFor(rawAddress, slot)) //
    , rccBitter(bandFor(RCCBASE | ((rbus - AHB1) << 2u), slot)) {}

  /** activate and release the module reset */

  // /** control the clock, off reduces chip power */
  // void setClockEnable(bool on = true) const;
  // /** @returns whether clock is on */
  // bool isEnabled(void) const;
  // /** reset and enable clock */
  // void init(void) const;
  // /** get base clock for this module */
  // u32 getClockRate(void) const;
  // /** @returns address of a register, @param offset is value from st's manual (byte address) */
  // volatile u32 *registerAddress(unsigned int offset) const {
  //   return & reinterpret_cast<u32 *>( blockAddress)[offset>>2]; //compiler sees offset as an array index .

  void reset() const {
    ControlWord resetter(rccBit(resetOffset));
    resetter = 1;
    resetter = 0; //manual is ambiguous as to whether reset is a command or a state.
  }

  /** control the clock, off reduces chip power */
void setClockEnable(bool on = true) const {
    ControlWord (rccBit(clockOffset)) = on;
  }

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
    return ControlWord(bandFor(blockAddress + offset, bit));//bandAddress and bandFor were both setting the 0200 0000 bit.
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
    rbus = stbus    //
   /** base device address @see registerAddress() for multi-bit control */
    , blockAddress = APB_Block(rbus, slot)    //
   /** base bit band address. @see bit() to access a single bit control */
    , bandAddress = APB_Band(rbus, slot)      //
   /** base used for calculating this device's bits in RCC device. */
    , rccBitat = bandFor(RCCBASE | (rbus << 2), slot)
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
  uint32_t getClockRate() const {
    return clockRate(rbus);
  }
};

#pragma clang diagnostic pop