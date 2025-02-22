#pragma once
//F4 gpio, significantly different configuration mechanism than F1, same data access but at different offsets.
#include "stm32.h"

/** Port is the 16 bits as a group, and doesn't have much to it other than factories for pins.
  * Note well that even when the group is not enabled the port can be read from (as long as it exists).
  *
  * For pins that are optional to a module use (const Pin *) parameters to pass nulls. Trying to create safely non-functional pins is expensive and the check for 'has a pin' is the same cost, but only burdens those pin uses which can be optionally present. There are usually some port bits that aren't pinned out which can be used as dummies when a null pointer to a pin just isn't convenient.
  */

/* The goal for this class is to have an array of these in ROM which can be iterated over to init all pins efficiently.
Each enum is sized for ROM efficiency.
Most enums are carefully ordered to go directly into a bitfield without translation. */


    /** port A is 0 slot in the RCC. We use this function so that the layer that changes a letter into an index can drift as we evolve this library */
  constexpr unsigned portNumber(unsigned portIndex) {
    return portIndex >= 'A' ? portIndex - 'A' : portIndex;
  }

  struct PinDeclaration {
    PinDeclaration(const PinDeclaration &other) = default;

    PinDeclaration(PinDeclaration &&other) noexcept = default;

    //bool isInput;//else output
    //bool activeHigh;//is relevant to pullup/down on some devices, pull the opposite of this level.

    enum Puller  {
      Float
      , Down
      , Up
      , NotPulled
    };
    //Puller UDF;

    //bool isFunction;//else gpio

    enum AfCode:uint8_t {
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
      EventOut,  //end of table.
      Not_AF
    };
    // AfCode altFunctionCode;//could be 4 bits if we wished.

    enum Slew:uint8_t  {
      slow = 0
      , medium
      , fast
      , fastest //#value is for control register
    };
    // Slew slew;

    struct {
      const unsigned portIndex:4;//no chip of interest has 16 ports, that would be 256 IO/pins and you are in a different processor class.
      const unsigned bitnum:4; //16 bit ports in this beast.

      const bool isInput:1;
      const bool activeHigh:1;//is relevant to pullup/down on some devices, pull the opposite of this level. rp2040 puts this on the pad, not the gpio!
      const bool isFunction:1;//else gp io
      /** isInput? analog input: open drain output*/
      const bool openDrain:1;

      const Puller UDF:2;//todo: replace with boolean 'pulled' and force pull to opposite of activeHigh.
      const Slew slew:2;

      const AfCode altFunctionCode:4;
    };

   constexpr PinDeclaration(unsigned portIndex, unsigned bitnum,const bool isInput,bool activeHigh, const Puller udfo, const bool isFunction, const AfCode altFunctionCode, const bool openDrain, const Slew slew) :
      portIndex{portNumber(portIndex)},bitnum{bitnum},
      isInput{isInput},
      activeHigh{activeHigh},
      UDF{udfo},
      isFunction{isFunction},
      altFunctionCode{altFunctionCode},
      openDrain{openDrain},
      slew{slew} {}

    void configure() const;

    //When you use the following factories remember to invoke config() on the object, these are constexpr and can't do anything but generate bit patterns in rom.
    static constexpr PinDeclaration Input(unsigned portIndex, unsigned bitnum,bool activeHigh,Puller UDFO = Float,AfCode altFun=Not_AF) {
      return PinDeclaration(portIndex, bitnum,true, activeHigh,UDFO, altFun!=Not_AF,altFun,false,slow);
    }

    static constexpr PinDeclaration Analog(unsigned portIndex, unsigned bitnum) {
     return PinDeclaration(portIndex, bitnum,true, false, Float, false,Not_AF,true,slow);
   }

    static constexpr PinDeclaration Output(unsigned portIndex, unsigned bitnum,bool activeHigh,Slew slew = slow, bool floater=false, AfCode altFun=Not_AF) {
      return PinDeclaration(portIndex, bitnum,false,activeHigh,floater?NotPulled:Float,altFun!=Not_AF,altFun,floater,slew);
    }

    static constexpr PinDeclaration Function(unsigned portIndex, unsigned bitnum,AfCode altcode, Slew slew = slow, Puller UDFO = NotPulled) {
      return PinDeclaration(portIndex, bitnum,false,true, UDFO,true, altcode,UDFO!=NotPulled, slew);
    }

  }__attribute__((packed));


struct Port /*Manager*/ : APBdevice {
  static constexpr unsigned gpiobase(unsigned Ais0) {
    return GPIOBASE + 0x400 * Ais0;
  }

  /** @param letter is the uppercase character from the stm32 manual or already converted to 0..10 or so index. */
  explicit constexpr Port(unsigned letter) : APBdevice(AHB1, portNumber(letter), gpiobase(portNumber(letter))) {}


  /** configure the given pin. */
  void configure(const PinDeclaration &c) const;

  constexpr ControlField pinField(Address offset, unsigned bitnum, unsigned width) const;

  /** a contiguous set of bits in a single Port */
  struct Field {
    const PinDeclaration lsb;
    const unsigned mask; //derived from width
    const Address odr;
    const Address at;

    constexpr Field(const PinDeclaration &pindef, unsigned msb):lsb(pindef)
    , mask(fieldMask(msb, lsb.bitnum) | fieldMask(msb, lsb.bitnum) << 16)
    , odr(Port(lsb.portIndex).registerAddress(0x14))
    , at(Port(lsb.portIndex).registerAddress(0x18))  //bsrr "bit set/reset register"
    {}

    /** insert @param value into field, shifting and masking herein, i.e always lsb align the value you supply here */
    unsigned operator=(unsigned value) const; // NOLINT(misc-unconventional-assign-operator,cppcoreguidelines-c-copy-assignment-signature)
    /** toggle the bits in the field that correspond to ones in the @param value */
    void operator^=(unsigned value) const;

    /** @returns the value set by the last operator =, ie the requested output- not what the physical pins are reporting. */
    operator uint16_t() const { // NOLINT(google-explicit-constructor)
      return (Ref<uint16_t>(odr) & mask) >> lsb.bitnum;
    }
    /** read back the actual pins */
    uint16_t actual() const;
    //more operators only at need
  };// Port::Field

  /** @param pincode is the same as for pin class */
  void configure(const Field&field ,const PinDeclaration &portcode) const;

};

// //using this macro as we have gyrated over how to specify ports:
// #define DefinePort(letter) const Port P##letter InitStep(InitHardware + 5)(*#letter)
// //the above macro is why people hate C. The '*' picks out the first letter of the string made by # letter, since the preprocessor insisted on honoring single ticks while parsing the #defined text.
//
// DefinePort(A);
// DefinePort(B);
// DefinePort(C);
// DefinePort(D);
// DefinePort(E);
// DefinePort(F);
// DefinePort(G);
// DefinePort(H);
// DefinePort(I);
// DefinePort(J);
//


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
  * If you have a runtime reason to change the definition of "active" then declare two pins with the same physical designator and swap between them at points of use.
  */
struct Pin /*Manager*/ {
  const ControlBit reader;
  const ControlBit writer;
  /** the level that is deemed active  */
  const bool active;

  constexpr Pin(const PinDeclaration &pindef) :
    reader(Port(pindef.portIndex). registerAddress(0x10), pindef.bitnum),
    writer(Port(pindef.portIndex). registerAddress(0x14), pindef.bitnum),
    active{pindef.activeHigh}{}

  /** get physical value for logical level*/
  bool polarized(bool operand) const {
    return active == operand;
  }

/** raw access convenience. @see InputPin for business logic version of a pin */
  operator bool() const { // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    return polarized(reader); //false lint warning about endless loop
  }

/** @returns pass through @param truth after setting pin to that value.
@see OutputPin for business logic version */
  //INLINETHIS
  bool operator=(bool truth) const { // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
    writer = polarized(truth);
    return truth;//#don't reread the pin, nor its actual, keep this as a pass through
  }


  bool actual() const {
    return polarized((&writer)[-32]);//idr is register preceding odr, hence 32 bits lower in address
  }

  /** set to given value, @returns whether a change actually occurred.*/
  bool changed(bool truth) const {
    truth = polarized(truth);
    if (actual() != truth) {
      writer = truth;
      return true;
    }
    return false;
  }

  /** actually invert the present state of the pin */
  void toggle() const;

};
