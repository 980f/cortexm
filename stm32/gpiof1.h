#pragma once

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#include "stm32.h"

/** the 16 bits as a group.
  * Note well that even when the group is not enabled the port can be read from (as long as it exists).
  *
  * For pins that are optional to a module use (const Pin *) parameters to pass nulls. Trying to create safely non-functional pins is expensive and the check for 'has a pin' is the same cost, but only burdens those pin uses which can be optionally present. There are usually some port bits that aren't pinned out which can be used as dummies when a null pointer to a pin just isn't convenient.
  *
  * todo:1 add Bidirectional pin with interface for dyanmic switching during runtime.
  * todo:1 abstract base classes across vendors to ensure names don't diverge.
  */
struct Port /*Manager*/ : public APBdevice {
  static constexpr unsigned gpiobase(unsigned Ais0) {
    return GPIOBASE + 0x400 * Ais0;
  }

  struct PinOptions {//using struct as namespace
    static constexpr unsigned input(bool analog = false, bool floating = false) {
      return analog ? 0 : (floating ? 4 : 8);//only 3 combos are legal
    }

    enum Slew {
      medium = 1, slow = 2, fast = 3
    };

    static constexpr unsigned output(Slew slew = slow, bool function = false, bool open = false) {
      return slew + (function ? 8 : 0) + (open ? 4 : 0);
    }

    unsigned code;

    explicit PinOptions(unsigned code) : code(code) {}

    //default copy etc are fine.
    static PinOptions Input(bool analog = false, bool floating = false) {
      return PinOptions(input(analog, floating));
    }

    static PinOptions Output(Slew slew = slow, bool function = false, bool open = false) {
      return PinOptions(output(slew, function, open));
    }
  };


  static bool isOutput(unsigned pincode);

  /** a contiguous set of bits in a single Port */
  struct Field {
    const Address odr;
    const Address at;
    const unsigned lsb;
    const unsigned mask; //derived from width
    const Port &port;

    constexpr Field(const Port &port, unsigned lsb, unsigned msb);

    /** @param pincode is the same as for pin class */
    void configure(unsigned pincode){
      // and actually set the pins to their types
      for(unsigned abit = lsb; mask&(1u<<abit); ++abit) {
        port.configure(abit, pincode);
      }
    }
    /** insert @param value into field, shifting and masking herein, i.e always lsb align the value you supply here */
    void operator=(unsigned value) const; // NOLINT(misc-unconventional-assign-operator,cppcoreguidelines-c-copy-assignment-signature)
    /** toggle the bits in the field that correspond to ones in the @param value */
    void operator^=(unsigned value) const;

    /** @returns the value set by the last operator =, ie the requested output- not what the physical pins are reporting. */
    operator u16() const; // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    /** read back the actual pins */
    u16 actual() const;
    //more operators only at need
  };

  /** @param letter is the uppercase character from the stm32 manual */
  explicit constexpr Port(char letter): APBdevice(AHB1, unsigned(letter - 'A'), gpiobase(letter - 'A')) {
    //#nada
  }


  /**
    * configure the given pin.
    todo:M enumerize the pin codes (but @see InputPin and OutputPin classes which construct codes for you.)
    */
  void configure(unsigned bitnum, unsigned code) const {
    if (!isEnabled()) { // deferred init, so we don't have to sequence init routines, and so we can statically create objects without wasting power if they aren't needed.
      init(); // must have the whole port running before we can modify a config of any pin.
    }
    const ControlField confword(registerAddress(bitnum & 8 ? 4 : 0), (bitnum & 7) << 2, 4);// &7:modulo 8, number of conf blocks in a 32 bit word.; 4 bits each block
    confword = code;
  }

//  /** @returns accessor object for "output data register" */
//  constexpr ControlWord odr() const {
//    return ControlWord(registerAddress(12));
//  }
};

/* configure and Port::Field expect a 4 bit code that is built as follows:
For inputs 0 for analog, 4 for floating 8 for biased 
For outputs +4 for open drain +8 for alt function + 1 for 10Mhz, 2 for 2 Mhz, 3 for 50 Mhz.

analog, floating, biased 

opendrain, function, slow, medium, fast

Portcode::analog
Portcode::output(unsigned 10,2,50,bool function=false, bool open=false){
}

*/


//These are being put into ram, so having multiple copies has been retracted.

extern const Port PA;
extern const Port PB;
extern const Port PC;
extern const Port PD;
extern const Port PE;

//GPIO interrupt configuration options. Some devices may not support some options, but most do so this is defined here.
enum IrqStyle {
  NotAnInterrupt = 0, // in case someone forgets to explicitly select a mode
  AnyEdge, // edge, either edge, input mode buslatch
  LowActive, // level, pulled up
  HighActive, // level, pulled down
  LowEdge, // edge, pulled up
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
  const ControlField confword;

  /** configure as output using given options */
  const Pin & output(unsigned int code, Port::PinOptions::Slew slew, bool openDrain) const {
    code |= openDrain << 2;
    switch (slew) {
    default: // on any errors be a slow output
    case Port::PinOptions::Slew::slow:
      code |= 2;
      break;
    case Port::PinOptions::Slew::medium:
      code |= 1;
      break;
    case Port::PinOptions::Slew::fast:
      code |= 3;
      break;
    }
    configureAs(code);
    return *this;
  }

  constexpr Pin(const Port &port, unsigned bitnum) :
    bitnum(bitnum), port(port), reader(port.registerAddress(8), bitnum), writer(port.registerAddress(12), bitnum),
    confword(port.registerAddress(bitnum & 8 ? 4 : 0), (bitnum & 7) << 2, 4)// &7:modulo 8, number of conf blocks in a 32 bit word.; 4 bits each block
  {
    /*empty*/
  }

/** @returns this after configuring it for analog input */
  const Pin &AI() const;

/** @returns bitband address for input after configuring as digital input, pull <u>U</u>p, pull <u>D</u>own, or leave <u>F</u>loating*/
  const Pin &DI(char UDF = 'D') const;

///** @returns bitband address for controlling high drive capability [rtfm] */
//  ControlWord highDriver() const;

/** configure as simple digital output */
  const Pin &DO(Port::PinOptions::Slew slew = Port::PinOptions::Slew::slow, bool openDrain = false) const {
    return output(0, slew, openDrain);
  }

/** configure pin as alt function output*/
  const Pin &FN(Port::PinOptions::Slew slew = Port::PinOptions::Slew::slow, bool openDrain = false) const;

//  /** declare your variable volatile, reads the actual pin, writing does nothing */
//  constexpr ControlWord reader() const;
//
//  /** @returns reference for writing to the physical pin, reading this reads back the DESIRED output */
//  constexpr ControlWord writer() const;

/** for special cases, try to use one of the above which all call this one with well checked argument */
  const Pin &configureAs(unsigned code) const {
    port.configure(bitnum, code);
    return *this;
  }

/** raw access convenience. @see InputPin for business logic version of a pin */
  operator bool() const { // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    return reader;
  }

/** @returns pass through @param truth after setting pin to that value.
@see OutputPin for business logic version */
  bool operator=(bool truth) const { // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
    writer = truth;
    return truth;//#don't reread the pin, nor its actual, keep this as a pass through
  }
};

/** base class for InputPin and OutputPin that adds polarity at construction time.
 //not templated as we want to be able to pass Pin's around. not a hierarchy as we don't want the runtime cost of virtual table lookup. */
class LogicalPin {
protected:
  const Pin &pin;
  /** the level that is deemed active  */
  const bool active;

  bool polarized(bool operand) const {
    return active != operand;
  }

//
//  explicit constexpr LogicalPin(Address registerAddress, bool active = true) :
//    bitbanger(registerAddress),
//    active(active) {
//    /*empty*/
//  }
  explicit constexpr LogicalPin(const Pin &pin, bool active = true) :
    pin(pin),
    active(active) {
    /*empty*/
  }

public:

  /** @returns for outputs REQUESTED state of pin, for inputs the actual pin */
  operator bool() const { // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    return polarized(pin);
  }
};

/**
hide the volatile and * etc that I sometimes forget.
*/
class InputPin : public LogicalPin {

public:
  explicit InputPin(const Pin &pin, char UDF = 'D', bool active = true);

  explicit InputPin(const Pin &pin, bool active);  //pull the opposite way of the 'active' level.
  //maydo: add method to change pullup/pulldown bias while running

};

/**
a digital output made to look like a simple boolean.
Note that these objects can be const while still manipulating the pin.
*/
class OutputPin : public LogicalPin {
public:
  explicit OutputPin(const Pin &pin, bool active = true, Port::PinOptions::Slew slew = Port::PinOptions::Slew::slow, bool openDrain = false) :
    LogicalPin(pin, active) {
    pin.DO(slew, openDrain);
  }

  /** @returns pass through @param truth after setting pin to that value */
  bool operator=(bool truth) const { // NOLINT (cppcoreguidelines-c-copy-assignment-signature)
    pin = polarized(truth);
    return truth;//don't reread the pin, nor its actual, keep this as a pass through
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

  bool actual() const {//todo:00 see if this survived ControlWord class.
    return polarized((&pin.writer)[-32]);//idr is register preceding odr, hence 32 bits lower in address
  }

  /** actually invert the present state of the pin */
  void toggle() const;
};

#pragma clang diagnostic pop
