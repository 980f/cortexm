#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "misc-unconventional-assign-operator"
#pragma ide diagnostic ignored "modernize-use-nodiscard"
#pragma once
//F4 gpio, significantly different configuration mechanism than F1, same data access but at different offsets.


#include "stm32.h"

#include "utility.h"

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

  constexpr explicit PinOptions(Dir dir, Puller UDFO = Float, Slew slew = slow, unsigned altcode = 0) : dir(dir), slew(slew), UDFO(UDFO), altcode(altcode) {
    //#nada
  }

  static PinOptions Input(Puller UDFO = Float) {
    return PinOptions(input, UDFO, slow);
  }

  static PinOptions Output(Slew slew = slow, Puller UDFO = Float) {
    return PinOptions(PinOptions::output, UDFO, slew);
  }

  static PinOptions Function(unsigned altcode, Slew slew = slow, Puller UDFO = Float) {
    return PinOptions(PinOptions::function, UDFO, slew, altcode);
  }

  unsigned altcode;//could be 4 bits if we wished.
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
  explicit constexpr Port(char letter) : APBdevice(AHB1, unsigned(letter - 'A'), gpiobase(letter - 'A')) {
  }

  /** configure the given pin. */
  void configure(unsigned bitnum, const PinOptions &c) const;

  void forAdc(unsigned bithum) const;

  /** a contiguous set of bits in a a single Port */
  struct Field {
    const unsigned lsb;
    const unsigned mask; //derived from width
    const Address odr;
    const Address at;
    const Port &port;

    constexpr Field(const Port &port, unsigned lsb, unsigned msb);

    /** insert @param value into field, shifting and masking herein, i.e always lsb align the value you supply here */
    void operator=(unsigned value) const; // NOLINT(misc-unconventional-assign-operator,cppcoreguidelines-c-copy-assignment-signature)
    /** toggle the bits in the field that correspond to ones in the @param value */
    void operator^=(unsigned value) const;

    /** @returns the value set by the last operator =, ie the requested output- not what the physical pins are reporting. */
    operator uint16_t() const { // NOLINT(google-explicit-constructor)
      return (Ref<u16>(odr) & mask) >> lsb;
    }
    /** read back the actual pins */
    u16 actual() const;
    //more operators only at need
  };// Port::Field


  /** @param pincode is the same as for pin class */
  void configure(const Field&field ,const PinOptions &portcode) const;

};

// priority must be such that these get created before any application objects, +5 make it be after clocks.
#define DefinePort(letter) const Port P##letter InitStep(InitHardware + 5)(*#letter)
//the above macro is why people hate C. The '*' picks out the first letter of the string made by # letter, since the preprocessor insisted on honoring single ticks while parsing the #defined text.

DefinePort(A);
DefinePort(B);
DefinePort(C);
DefinePort(D);
DefinePort(E);
DefinePort(F);
DefinePort(G);
DefinePort(H);
DefinePort(I);
DefinePort(J);
////these take up little space and it gets annoying to have copies in many places.
//extern const Port PA;
//extern const Port PB;
//extern const Port PC;
//extern const Port PD;
//extern const Port PE;
//extern const Port PF;
//extern const Port PG;
//extern const Port PH;
//extern const Port PI;
//const Port PJ{'J'};

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
struct Pin /*Manager*/ {
  const unsigned bitnum;
  const Port &port;
  const ControlBit reader;
  const ControlBit writer;

  constexpr Pin(const Port &port, unsigned bitnum) :
    bitnum(bitnum), port(port), reader(port.registerAddress(0x10), bitnum), writer(port.registerAddress(0x14), bitnum) {
    //#nada
  }

/** @returns this after configuring it for analog input */
  const Pin &AI() const;

/** @returns bitband address for input after configuring as digital input, default pulldown as that is what test equipment induces ;) */
  const Pin &DI(PinOptions::Puller UDFO = PinOptions::Down) const;

/** configure as simple digital output */
  const Pin &DO(PinOptions::Slew slew = PinOptions::Slew::slow, PinOptions::Puller UDFO = PinOptions::Down) const;

/** configure pin as alt function output*/
  const Pin &FN(unsigned nibble, PinOptions::Slew slew = PinOptions::Slew::slow, PinOptions::Puller UDFO = PinOptions::Float) const;

/** raw access convenience. @see InputPin for business logic version of a pin */
  //INLINETHIS
  operator bool() const { // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    return reader; //false lint warning about endless loop
  }

/** @returns pass through @param truth after setting pin to that value.
@see OutputPin for business logic version */
  //INLINETHIS
  bool operator=(bool truth) const { // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
    writer = truth;
    return truth;//#don't reread the pin, nor its actual, keep this as a pass through
  }
};

/** @deprecated  use a pinconfigurator and a logical pin
 * declare a pin used by a peripheral, one that will not get directly manipulated but might be inspectable. */
struct FunctionPin {
  const ControlBit reader;

  operator bool() const { // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    return reader;
  }

  //cannot be constexpr as it hits the configuration registers and that takes real code.
  FunctionPin(const Port &port, unsigned bitnum, unsigned nibble, PinOptions::Slew slew = PinOptions::Slew::slow, PinOptions::Puller UDFO = PinOptions::Float) :
    reader(port.registerAddress(0x10), bitnum) {
    port.configure(bitnum, PinOptions(PinOptions::function, UDFO, slew, nibble));
  }
};

/** base class for InputPin and OutputPin that adds polarity at construction time.
 //not templated as we want to be able to pass Pin's around. not a hierarchy as we don't want the runtime cost of virtual table lookup.
 As of addition of pinconfigurator this is becoming a directly usable class */
class LogicalPin {
protected:
  const Pin pin; //removed reference as pins were sometimes created inside a parameter list, and as such evaporated.
  /** the level that is deemed active  */
  const bool active;

public:
  //INLINETHIS
  bool polarized(bool operand) const {
    return active == operand;
  }
  //INLINETHIS
  explicit constexpr LogicalPin(Pin &&pin, bool active = true) :
    pin(std::move(pin)), active(active) {
    //#nada
  }

public:
  //INLINETHIS
  /** @returns for outputs REQUESTED state of pin, for inputs the actual pin */
  operator bool() const { // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    return polarized(pin);
  }

  //INLINETHIS
  /** @returns pass through @param truth after setting pin to that value */
  bool operator=(bool truth) const { // NOLINT (cppcoreguidelines-c-copy-assignment-signature)
    pin = polarized(truth);
    return truth;//don't reread the pin, nor its actual, keep this as a pass through
  }
};

/** @deprecated  use a pinconfigurator and a logical pin
A pin configured and handy to use for logical input, IE the polarity of "1" is set by the declaration not by each point of use.
*/
class InputPin : public LogicalPin {

public:
  constexpr explicit InputPin(Pin &&pin, PinOptions::Puller UDF = PinOptions::Down, bool active = true) : LogicalPin(std::move(pin), active) {
    pin.DI(UDF);
  }

  /** pull the opposite way of the 'active' level. */
  constexpr explicit InputPin(Pin &&pin, bool active) : InputPin(std::move(pin), active ? PinOptions::Down : PinOptions::Up, active) {
    //#nada
  }

//  InputPin(const InputPin &copyme) = default;
};

/** @deprecated  use a pinconfigurator and a logical pin
a digital output made to look like a simple boolean.
Note that these objects can be const while still manipulating the pin.
*/
class OutputPin : public LogicalPin {
public:
  constexpr explicit OutputPin(Pin &&pin, bool active = true, PinOptions::Slew slew = PinOptions::Slew::slow, bool openDrain = false) :
    LogicalPin(std::move(pin), active) {
    pin.DO(slew, openDrain ? PinOptions::OpenDrain : PinOptions::Float);
  }

  /** set to given value, @returns whether a change actually occurred.*/
  bool changed(bool truth) const {
    truth = polarized(truth);
    if (pin != truth) {
      pin = truth;
      return true;
    }
    return false;
  }

  bool actual() const {
    return polarized((&pin.writer)[-32]);//idr is register preceding odr, hence 32 bits lower in address
  }

  /** actually invert the present state of the pin */
  void toggle() const;
};

#pragma clang diagnostic pop