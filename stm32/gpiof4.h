#pragma once
//F$ gpio, significantly different configuration mechanism, same data access but at different offsets.
#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#include "stm32.h"

struct PinOptions {
 
  enum Dir {
    input=0,output,function,analog  //#value is for control register
  };

  Dir dir; 

 enum Slew {
    slow=0, medium , fast , fastest //#value is for control register
  };

  Slew slew;

//cheap enum for pullup/down/float/open_drain
  char UDFO;

  PinOptions(Dir dir,Slew slew=slow,char UDFO='F'):dir(dir),slew(slew),UDFO(UDFO){
    //#nada
  }

  static PinOptions Input(char UDFO='F') {
    return PinOptions(input,slow,UDFO);
  }

  static PinOptions Output(Slew slew = slow, bool function = false, char UDFO='F') {
    return PinOptions(function ? PinOptions::function : PinOptions::output,slew,UDFO);
  }
};
  

/** the 16 bits as a group.
  * Note well that even when the group is not enabled the port can be read from (as long as it exists).
  *
  * For pins that are optional to a module use (const Pin *) parameters to pass nulls. Trying to create safely non-functional pins is expensive and the check for 'has a pin' is the same cost, but only burdens those pin uses which can be optionally present. There are usually some port bits that aren't pinned out which can be used as dummies when a null pointer to a pin just isn't convenient.
  */

struct Port /*Manager*/ : public APBdevice {

  /** @param letter is the uppercase character from the stm32 manual */
  explicit constexpr Port(char letter);

  /**
    * configure the given pin.
    todo:M enumerize the pin codes (but @see InputPin and OutputPin classes which construct codes for you.)
    */
  void configure(unsigned bitnum, const PinOptions &c) const ;

  /** a contiguous set of bits in a a single Port */
  struct Field {
    const Address odr;
    const Address at;
    const unsigned lsb;
    const unsigned mask; //derived from width
    const Port &port;

    constexpr Field(const Port &port, unsigned lsb, unsigned msb);

    /** @param pincode is the same as for pin class */
    void configure(const PinOptions &portcode ){
      // and actually set the pins to their types
      for(unsigned abit = lsb; mask&(1u<<abit); ++abit) {
        port.configure(abit, portcode);
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
  };// Port::Field

};



//these take up little space and it gets annoying to have copies in many places.
extern const Port PA;
extern const Port PB;
extern const Port PC;
extern const Port PD;
extern const Port PE;
extern const Port PF;
extern const Port PG;
extern const Port PH;
extern const Port PI;
extern const Port PJ;

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
  const ControlWord reader;
  const ControlWord writer;

  Pin(const Port &port, unsigned bitnum) :
    bitnum(bitnum), port(port), reader(port.registerAddress(0x10), bitnum), writer(port.registerAddress(0x14), bitnum){
    //#nada
  }

/** @returns this after configuring it for analog input */
  const Pin &AI() const;

/** @returns bitband address for input after configuring as digital input, pull <u>U</u>p, pull <u>D</u>own, or leave <u>F</u>loating*/
  const Pin &DI(char UDFO = 'D') const;

/** configure as simple digital output */
  const Pin &DO(PinOptions::Slew slew = PinOptions::Slew::slow, char UDFO = 'D') const {
    port.configure(bitnum, PinOptions(PinOptions::output, slew, UDFO));
    return *this;
  }

/** configure pin as alt function output*/
  const Pin &FN(unsigned nibble,PinOptions::Slew slew = PinOptions::Slew::slow, char UDFO = 'D') const;

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

  explicit constexpr LogicalPin(const Pin &pin, bool active = true) :
    pin(pin),
    active(active) {
    //#nada
  }

public:

  /** @returns for outputs REQUESTED state of pin, for inputs the actual pin */
  operator bool() const { // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    return polarized(pin);
  }
};

/**
A pin configured and handy to use for logical input, IE the polarity of "1" is set by the declaration not by each point of use.
*/
class InputPin : public LogicalPin {

public:
  explicit constexpr InputPin(const Pin &pin, char UDF = 'D', bool active = true): LogicalPin(pin, active) {
    pin.DI(UDF);
  }
  /** pull the opposite way of the 'active' level. */
  explicit constexpr InputPin(const Pin &pin, bool active):InputPin(pin, active ? 'D' : 'U', active) {
    //#nada
  }

};

/**
a digital output made to look like a simple boolean.
Note that these objects can be const while still manipulating the pin.
*/
class OutputPin : public LogicalPin {
public:
  explicit constexpr OutputPin(const Pin &pin, bool active = true, PinOptions::Slew slew = PinOptions::Slew::slow, bool openDrain = false) :
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
