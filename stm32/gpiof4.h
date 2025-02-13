#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "misc-unconventional-assign-operator"
#pragma ide diagnostic ignored "modernize-use-nodiscard"
#pragma once
//F4 gpio, significantly different configuration mechanism than F1, same data access but at different offsets.


#include "stm32.h"

#include "utility.h"


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
  explicit constexpr Port(char letter) : APBdevice(AHB1, unsigned(letter - 'A'), gpiobase(letter - 'A')) {}

/* The goal for this class is to have an array of these in ROM which can be iterated over to init all pins efficiently.
Each enum is sized for ROM efficiency.
Each enum is carefully ordered to go directly into a bitfield without translation. */
  struct PinOptions {

    enum Dir:uint8_t  {
      input = 0
      , output
      , function
      , analog 
    };
    Dir dir;

    enum Slew:uint8_t  {
      slow = 0
      , medium
      , fast
      , fastest //#value is for control register
    };
    Slew slew;

    enum Puller:uint8_t  {
      Float
      , Down
      , Up
      , OpenDrain  //# ordered for ease of setting up/down registers
    };
    Puller UDFO;


    enum Altfunc:uint8_t {
      SYS_AF=0,
      Timer1r2,
      Timer345,
      Timer9orAbove,
      I2C,
      SSP2r3,
      SSP345,
      Uart1r2rSSP3,
      Uart6,
      I2Ctoo,
      USB_FS,
      AF11,
      SDIO,
      AF13,AF14,
      EventOut  //end of table.
    };
    Altfunc altcode;//could be 4 bits if we wished.

    constexpr PinOptions(Dir dir, Puller UDFO = Float, Slew slew = slow, Altfunc altcode = SYS_AF) : dir(dir), slew(slew), UDFO(UDFO), altcode(altcode) {}

    static constexpr PinOptions Input(Puller UDFO = Float) {
      return PinOptions(input, UDFO, slow);
    }

    static constexpr PinOptions Output(Slew slew = slow, Puller UDFO = Float) {
      return PinOptions(PinOptions::output, UDFO, slew);
    }

    static constexpr PinOptions Function(Altfunc altcode, Slew slew = slow, Puller UDFO = Float) {
      return PinOptions(PinOptions::function, UDFO, slew, altcode);
    }

  }__attribute__((packed));

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
    unsigned operator=(unsigned value) const; // NOLINT(misc-unconventional-assign-operator,cppcoreguidelines-c-copy-assignment-signature)
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

//using this macro as we have gyrated over how to specify ports:
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
  const Pin &DI(Port::PinOptions::Puller UDFO = Port::PinOptions::Down) const;

/** configure as simple digital output */
  const Pin &DO(Port::PinOptions::Slew slew = Port::PinOptions::Slew::slow, Port::PinOptions::Puller UDFO = Port::PinOptions::Down) const;

/** configure pin as alt function output*/
  const Pin &FN(unsigned nibble, Port::PinOptions::Slew slew = Port::PinOptions::Slew::slow, Port::PinOptions::Puller UDFO = Port::PinOptions::Float) const;

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
  FunctionPin(const Port &port, unsigned bitnum, Port::PinOptions::Altfunc nibble, Port::PinOptions::Slew slew = Port::PinOptions::Slew::slow, Port::PinOptions::Puller UDFO = Port::PinOptions::Float) :
    reader(port.registerAddress(0x10), bitnum) {
    port.configure(bitnum, Port::PinOptions(Port::PinOptions::function, UDFO, slew, nibble));
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
  explicit constexpr LogicalPin(const Pin &pin, bool active = true) :
    pin(pin), active(active) {
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
  constexpr explicit InputPin(const Pin &pin, Port::PinOptions::Puller UDF = Port::PinOptions::Down, bool active = true) : LogicalPin(pin, active) {
    pin.DI(UDF);
  }

  /** pull the opposite way of the 'active' level. */
  constexpr explicit InputPin(const Pin &pin, bool active) : InputPin(pin, active ? Port::PinOptions::Down : Port::PinOptions::Up, active) {
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
  constexpr explicit OutputPin(const Pin &pin, bool active = true, Port::PinOptions::Slew slew = Port::PinOptions::Slew::slow, bool openDrain = false) :
    LogicalPin(pin, active) {
    pin.DO(slew, openDrain ? Port::PinOptions::OpenDrain : Port::PinOptions::Float);
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

  using LogicalPin::operator=;
};

#pragma clang diagnostic pop
