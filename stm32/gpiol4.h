#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"
#pragma once
/* L4 gpio, very similar to F4 BUT not in bitband space (the bastards!)
 * we will have to replace the very optimal bitband access with the brss register mechanism formerly used only for fields vs single bits.
 * we will make a gpio baser class later, I need L452 running today.
 */

#include "stm32.h"
#if __has_include("utility")
#include <utility>
#else
#include "utility.h"
#endif

struct PinOptions {

  enum Dir {
    input = 0
    , output
    , function
    , analog  //#value is for control register
  };
  Dir dir;

  enum Slew {
    slow = 0
    , medium
    , fast
    , fastest //#value is for control register
  };
  Slew slew;

  enum Puller {
    Float
    , Down
    , Up
    , OpenDrain  //# ordered for ease of setting up/down registers
  };
  Puller UDFO;

  constexpr explicit PinOptions(Dir dir, Slew slew = slow, Puller UDFO = Float, unsigned altcode = 0) : dir(dir), slew(slew), UDFO(UDFO), altcode(altcode) {
    //#nada
  }

  static PinOptions Input(Puller UDFO = Float) {
    return PinOptions(input, slow, UDFO);
  }

  static PinOptions Output(Slew slew = slow, Puller UDFO = Float) {
    return PinOptions(PinOptions::output, slew, UDFO);
  }

  static PinOptions Function(unsigned altcode, Slew slew = slow, Puller UDFO = Float) {
    return PinOptions(PinOptions::function, slew, UDFO, altcode);
  }

  unsigned altcode;
};

/** the 16 bits as a group.
  * Note well that even when the group is not enabled the port can be read from (as long as it exists).
  *
  * For pins that are optional to a module use (const Pin *) parameters to pass nulls. Trying to create safely non-functional pins is expensive and the check for 'has a pin' is the same cost, but only burdens those pin uses which can be optionally present. There are usually some port bits that aren't pinned out which can be used as dummies when a null pointer to a pin just isn't convenient.
  */

struct Port /*Manager*/ : public APBdevice {
  static constexpr unsigned gpiobase(unsigned Ais0) {
    return GPIOBASE + 0x400 * Ais0;
  }

  /** @param letter is the uppercase character from the stm32 manual */
  constexpr Port(char letter) : APBdevice(AHB1, unsigned(letter - 'A'), gpiobase(letter - 'A')) {
    //#nada
  }

  /** configure the given pin. */
  void configure(unsigned bitnum, const PinOptions &c) const;

  void forAdc(unsigned bithum) const;

  /** a contiguous set of bits in a a single Port */
  struct Field {
    const Address idr;
    const Address at;
    const unsigned lsb;  //for shifts
    const unsigned mask; //derived from lsb and width
    const Port &port;

    /** lsb and msb are both inclusive , eg for bits 6,7,8,9 pass 6,9 */
    constexpr Field(const Port &port, unsigned lsb, unsigned msb);

    /** @param pincode is the same as for pin class */
    void configure(const PinOptions &portcode) {
      // and actually set the pins to their types
      for (unsigned abit = lsb; mask & (1u << abit); ++abit) {
        port.configure(abit, portcode);
      }
    }

    /** insert @param value into field, shifting and masking herein, i.e always lsb align the value you supply here */
    void operator=(unsigned value) const {   // NOLINT(misc-unconventional-assign-operator)
      ControlWord field(at);
      field = mask & (((((~value) << 16) | value)) << lsb);  // read the stm32 manual for this.
    }

    /** toggle the bits in the field that correspond to ones in the @param value */
    void operator^=(unsigned value) const;

    /** @returns the actual pin values */
    operator uint16_t() const { // NOLINT(google-explicit-constructor)
      return (Ref<u16>(idr) & mask) >> lsb;
    }
    //more operators only at need
  };// Port::Field

};

constexpr unsigned NumberOfPorts=8;
extern const Port Ports[NumberOfPorts];
/** given some address withing the port's space we can figure out which port it is*/
const Port &fromAddress(Address sompiece){
  unsigned index = (sompiece>>10)%NumberOfPorts;
  return Ports[index];
}

#define PA Ports[0]
#define PB Ports[1]
#define PC Ports[2]
#define PD Ports[3]
#define PE Ports[4]
#define PF Ports[5]
#define PG Ports[6]
#define PH Ports[7]


//GPIO interrupt configuration options. Some devices may not support some options, but most do so this is defined here.
enum IrqStyle {
  NotAnInterrupt = 0
  , // in case someone forgets to explicitly select a mode
  AnyEdge
  , // edge, either edge, input mode buslatch
  LowActive
  , // level, pulled up
  HighActive
  , // level, pulled down
  LowEdge
  , // edge, pulled up
  HighEdge   // edge, pulled down
};

/**
  * this class manages the nature of a pin, and provides cacheable accessors for the pin value.
  * you may declare each as const, the internals are all const.
  */
struct Pin /*Manager*/:Port::Field {

  constexpr Pin(const Port &port, unsigned bitnum) : Port::Field(port,bitnum,bitnum){
    //#nada
  }
//removed, use PinConfigurator class instead
///** @returns this after configuring it for analog input */
//  const Pin &AI() const;

/** @returns bitband address for input after configuring as digital input, default pulldown as that is what test equipment induces ;) */
  const Pin &DI(PinOptions::Puller UDFO = PinOptions::Down) const;

/** configure as simple digital output */
  const Pin &DO(PinOptions::Slew slew = PinOptions::Slew::slow, PinOptions::Puller UDFO = PinOptions::Down) const;

//deprecated by pinconfigurator class. We happen to not dynamically convert pins into function pins. And you can do that with a pinconfigurator.
///** configure pin as alt function output*/
//  const Pin &FN(unsigned nibble, PinOptions::Slew slew = PinOptions::Slew::slow, PinOptions::Puller UDFO = PinOptions::Float) const;

/** raw access convenience. @see InputPin for business logic version of a pin */
  INLINETHIS
  operator bool() const { // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    return (Ref<u16>(idr) >> lsb)!=0;
  }

/** @returns pass through @param truth after setting pin to that value.
@see OutputPin for business logic version */
  INLINETHIS // NOLINT(misc-unconventional-assign-operator)
  bool operator=(bool truth) const {
    ControlWord bsrs(at);
    bsrs = 1<<(truth? lsb:(lsb+16));  // read the stm32 manual for this.
    return truth;//#don't reread the pin, nor its actual, keep this as a pass through
  }
};

///** declare a pin used by a peripheral, one that will not get directly manipulated but might be inspectable. */
//struct FunctionPin {
//  const ControlBit reader;
//
//  operator bool() const { // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
//    return reader;
//  }
//
//  //cannot be constexpr as it hits the configuration registers and that takes real code.
//  FunctionPin(const Port &port, unsigned bitnum, unsigned nibble, PinOptions::Slew slew = PinOptions::Slew::slow, PinOptions::Puller UDFO = PinOptions::Float) :
//    reader(port.registerAddress(0x10), bitnum) {
//    port.configure(bitnum, PinOptions(PinOptions::function, slew, UDFO, nibble));
//  }
//};

/** base class for InputPin and OutputPin that adds polarity at construction time.
 //not templated as we want to be able to pass Pin's around. not a hierarchy as we don't want the runtime cost of virtual table lookup.
 As of addition of pinconfigurator this is becoming a directly usable class */
class LogicalPin {
protected:
  const Pin pin; //removed reference as pins were sometimes created inside a parameter list, and as such evaporated.
  /** the level that is deemed active  */
  const bool active;

public:
  INLINETHIS
  bool polarized(bool operand) const {
    return active == operand;
  }
  INLINETHIS
  explicit constexpr LogicalPin(Pin &&pin, bool active = true) :
    pin(pin), active(active) {
    //#nada
  }

public:
  INLINETHIS
  /** @returns for outputs REQUESTED state of pin, for inputs the actual pin */
  operator bool() const { // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    return polarized(pin);
  }

  INLINETHIS // NOLINT (cppcoreguidelines-c-copy-assignment-signature)
  /** @returns pass through @param truth after setting pin to that value */
  bool operator=(bool truth) const {
    pin = polarized(truth);
    return truth;//don't reread the pin, nor its actual, keep this as a pass through
  }

  void toggle() const {
    pin = !pin;  //we can ignore polarity stuff :)
  }

};

///**
//A pin configured and handy to use for logical input, IE the polarity of "1" is set by the declaration not by each point of use.
//*/
//class InputPin : public LogicalPin {
//
//public:
//  constexpr explicit InputPin(Pin &&pin, PinOptions::Puller UDF = PinOptions::Down, bool active = true) : LogicalPin(std::move(pin), active) {
//    pin.DI(UDF);
//  }
//
//  /** pull the opposite way of the 'active' level. */
//  constexpr explicit InputPin(Pin &&pin, bool active) : InputPin(std::move(pin), active ? PinOptions::Down : PinOptions::Up, active) {
//    //#nada
//  }
//
////  InputPin(const InputPin &copyme) = default;
//};

///**
//a digital output made to look like a simple boolean.
//Note that these objects can be const while still manipulating the pin.
//*/
//class OutputPin : public LogicalPin {
//public:
//  constexpr explicit OutputPin(Pin &&pin, bool active = true, PinOptions::Slew slew = PinOptions::Slew::slow, bool openDrain = false) :
//    LogicalPin(std::move(pin), active) {
//    pin.DO(slew, openDrain ? PinOptions::OpenDrain : PinOptions::Float);
//  }
//
//  /** set to given value, @returns whether a change actually occurred.*/
//  bool changed(bool truth) const {
//    truth = polarized(truth);
//    if (pin != truth) {
//      pin = truth;
//      return true;
//    }
//    return false;
//  }
//
////  bool actual() const {
////    return polarized((&pin.writer)[-32]);//idr is register preceding odr, hence 32 bits lower in address
////  }
//
//  /** actually invert the present state of the pin */
//  void toggle() const;
//};

#pragma clang diagnostic pop