#pragma once

#include "lpcperipheral.h"
#include "cheaptricks.h"
#include "boolish.h"
#include "bitbanger.h"

/** the ports are numbered from 0. Making them unsigned gives us a quick bounds check via a single compare*/
typedef u8 PortNumber;

/** we could constrain this for 0..11 */
typedef u8 BitNumber;

namespace LPC {
///** @returns whether port number is legal. This is so unlikely to ever fail that we quit using it. */
//constexpr bool isLegalPort(PortNumber pn){
//  return pn < 4;
//}

/** @returns block base address, 64k addresses per port */
constexpr unsigned portBase(PortNumber portNum){
  return 0x50000000 + (portNum << 16); // this is the only ahb device, and each gpio is 4 blocks thereof so just have a custom address computation.
}

/** @returns block control address, base + 32k . @param regOffset is the value from the LPC manual */
constexpr unsigned portControl(PortNumber portNum,unsigned regOffset){
  return portBase(portNum) | (1<<15) | regOffset; // this is the only ahb device, and each gpio is 4 blocks thereof so just have a custom address computation.
}

/** @returns linear index of pin (combined port and bit)
 *  this index is useful for things like figuring out which interrupt vector is associated with the pin. */
constexpr unsigned pinIndex(PortNumber portNum, BitNumber bitPosition){
  return portNum * 12 + bitPosition;
}

constexpr unsigned pinMask(unsigned pinIndex){
  return 1U<<(pinIndex%12);
}

/** there is no relationship between the ioconfiguration register for a pin and its gpio designation.
 *  the LPC designers should be spanked for this, spanked hard and with something nasty. */
constexpr u8 ioconf_map[] =
{ // pass this a pinIndex
  3, 4, 7, 11, 12, 13, 19, 20, 24, 25, 26, 29,
  30, 31, 32, 36, 37, 40, 41, 42, 5, 14, 27, 38,
  2, 10, 23, 35, 16, 17, 0, 8, 9, 21, 22, 28,
  33, 34, 39, 43, 15, 18,
  // 1 6 are not present in this list (reserved locations in hardware map), gok what they may be used for someday.
};


/** the pins for which this are true have different io configuration patterns than the rest.
 * they are the debug interface, defaulting to that use.
// 0.0 0.10 0.11   1.0 1.1 1.2 1.3 */
constexpr bool isDoa(unsigned pinIndex){
  return (15>= pinIndex && pinIndex >=10)||pinIndex==0;
}

constexpr bool canAnalog(unsigned pinIndex){
  return (16>= pinIndex && pinIndex >=11)||pinIndex==22||pinIndex==23;
}


constexpr unsigned gpioBankInterrupt(PortNumber portNum){
  return 56-portNum;
}

/** declared outside of InputPin class so that we don't have to apply template args to each use.
 * Note: this can also be viewed as 2 booleans, PullUp and PullDown, BusLatch= both of those.
*/
enum PinBias { //#ordered for MODE field of iocon register
  LeaveFloating = 0, // in case someone forgets to explicitly select a mode
  PullDown, // level, pulled down
  PullUp, // level, pulled up, most pins default to this
  BusLatch, // edge, either edge, input mode buslatch
};

/** this thing has been very hard to make convenient.
code must be used to set this value where it belongs, embedding the value in a constructor makes that code invisible.
 to get this module into service I am going to restrict InptuPin and OutputPin to being just buslatch gpio. */
union PinSpec {
  u8 code;
  struct {
  unsigned fncode:3;
  unsigned pb:2;
  unsigned hysterical:1;
  //bit 6 is always a 1
  unsigned digital:1;
  };
};

/** our choice of how digital pins should be configured */
constexpr u8 digitalPattern(bool doa){ return 0b11011000 + doa;}
/** how analog pins are configured */
constexpr u8 analogPattern(bool doa){ return 0b01000001+doa;}

constexpr unsigned *IoconRegister(unsigned moderegaddress){
  return & (reinterpret_cast<unsigned *>(LPC::apb0Device(17))[moderegaddress]);
}


/** one form of access to a pin, use where the pin selection is runtime option. */
class GPIO :public BoolishRef {
protected:
  /** address associated with single bit mask */
  unsigned & dataAccess;
public:
  const unsigned pini;
  static constexpr unsigned baseAddress(PortNumber portNum, BitNumber bitPosition){
    return (portBase(portNum)+((1U<<bitPosition)<<2));
  }

  static constexpr unsigned baseAddress(unsigned pinIndex){
    return baseAddress(pinIndex/12,pinIndex%12);
  }

public:
  GPIO(PortNumber portNum, BitNumber bitPosition):
    dataAccess(*atAddress(baseAddress(portNum,bitPosition))),
    pini(pinIndex(portNum, bitPosition))
  {
    //#nada
  }

  /** set like writing to a boolean, @returns @param setHigh, per BoolishRef requirements*/
  bool operator =(bool setHigh) const {
    dataAccess = 0-setHigh;//all ones for setHigh, all zeroes for !setHigh, address picks the bit.
    return setHigh;
  }

  /** read like a boolean, @returns 1 or 0, per BoolishRef requirements*/
  operator bool() const {
    return dataAccess!=0;
  }

  /** set the ioconfiguration for the given pin to the given pattern. Pattern must already be adjusted for doa */
  static void setIocon(unsigned pinIndex,u8 pattern){
    reinterpret_cast<unsigned *>(LPC::apb0Device(17))[ioconf_map[pinIndex]] = pattern;
  }

  /** this presumes ps is already adjusted for doa */
  void setIocon(u8 pattern)const{
    GPIO::setIocon(pini,pattern);
  }

public:
  /** this must be called once before any other functions herein are used. Declaring it to be in an init section is a nice way to guarantee that.
As of 2017jan14 SystemInit is calling this, that is simpler to maintain than an array of function pointers in an explicit section.
*/
  static void Init( void );

public: //interrupt stuff. The manual is very opaque about this stuff. The IRQ stuff here only feeds the shared-per-port interrupts. Individual vectoring is via the Start logic.
  // values for gpio config as well as irq config.

  /** for slightly faster control than calling the respective members of GPIO itself.
   * The 0x1C bias herein is due to that being the only register that will always be accessed in an ISR, and hence worthy of the greatest optimization.
   * The compiler when allowed to optimize should be able to inline all register operations with minimum possible code, if pin is declared statically.
  */
  class IrqControl {
    u32 *regbase;//direction register
    const u32 mask;//single bit mask
    inline void setRegister(unsigned offset)const{
      regbase[offset]|=mask;
    }
    inline void clearRegister(unsigned offset)const{
      regbase[offset]&=~mask;
    }
    inline void assignRegister(unsigned offset,bool level)const{
      if(level){
        setRegister(offset);
      } else {
        clearRegister(offset);
      }
    }

  public:
    IrqControl(const GPIO &gpio):
      regbase(atAddress(( ~bitMask(0,16) & (GPIO::baseAddress(gpio.pini) | (1<<15))))), //interrupt clear register
      mask(pinMask(gpio.pini))
    {}
    void setIrqStyle(IrqStyle style,bool andEnable)const;
    inline void irq(bool enable)const;
    /** clear pending bit*/
    inline void irqAcknowledge()const;
    /** for when you want to interrupt yourself */
    void setDirection(bool output)const;
  };

  void setDirection(bool output)const;
  void setIrqStyle(IrqStyle style,bool andEnable)const;
  void irq(bool enable)const;
  /** clear pending bit*/
  void irqAcknowledge()const;

};


class Output: public GPIO {
public:
  Output(PortNumber portNum, BitNumber bitPosition):GPIO(portNum,bitPosition){
    setIocon(digitalPattern(isDoa(pini)) );
    setDirection(1);
  }
  //grrr, should be able to inherit this, wtf C++?
  bool operator=(bool other)const{
    return GPIO::operator=(other);
  }

};

/** Multiple contiguous bits in a register, this presumes the bits are configured elsewhere via GpioPin objects,
 * this class just expedites access using the gpio port 'masked[]' based access
 */
class GpioField {
protected:
  // read the lpc manual, certain address bits are used as a mask
  const unsigned address;
  const unsigned lsb;
public:
  /** actively set as output else input. The iocon registers will also have to be configured seperately.*/
  void setDirection(bool forOutput)const;

  /** set all the pins associated with this field to the same configuration stuff. */
  void configurePins(u8 pattern)const{
    unsigned pinIndex= lsb+ ((address>>16)&3)*12;
    u16 picker=1<<(lsb+2);
    while(address & picker){
      GPIO::setIocon(pinIndex,pattern+isDoa(pinIndex));
      picker<<=1;
      ++pinIndex;
    }
  }

  /** port is 0..3, msb and lsb are 11..0 */
  GpioField (PortNumber portNum, unsigned msb, unsigned lsb);
  /** read actual pin values, might not match last output request if the pins are overloaded */
  inline operator unsigned() const {
    return *atAddress(address) >> lsb;
  }

  /** write data to pins, but only effective if an output, doesn''t cause a spontaneous reconfiguration */
  inline unsigned operator =(unsigned value) const {
    *atAddress(address) = value << lsb;
    return value;
  }
};

/** constructor for output field*/
struct GpioOutputField: public GpioField {
  GpioOutputField(PortNumber portNum, unsigned msb, unsigned lsb):GpioField(portNum,msb,lsb){
    configurePins(digitalPattern(0));//0: doa will get added when needed to the pattern we provide here.
    setDirection(true);
  }
  /** write data to pins, but only effective if an output, doesn''t cause a spontaneous reconfiguration */
  inline unsigned operator =(unsigned value) const {
    *atAddress(address) = value << lsb;
    return value;
  }
};

// and now for the modern approach:
/** to configure a pin for a dedicated function one must construct a PortPin with template args for which pin then call setIocon to select the function */
template <PortNumber portNum, BitNumber bitPosition> class PortPin: public BoolishRef {
public:
  enum {
    pini = pinIndex(portNum, bitPosition)
  };

protected: // for simple gpio you must use an extended class that defines read vs read-write capability.
  enum {//these are all compile time computer and take no space in the chip, except for inline references.
    mask = 1 << bitPosition, // used for port control register access
    base = portBase(portNum), // base for port control
    // some pins are special on reset, and this alters their function code setting
    doa = isDoa(pini),
    analogish = canAnalog(pini),
    mode = ioconf_map[pini], // iocon array index
    pinn = base + (mask << 2),    // physical pin 'masked' access location "address == pattern"
    ctrl = base + (1<<15)  //address of control block
  };


  /** @returns reference to the masked access port of the register, mask set to the one bit for this pin. @see InputPin and @see OutputPin classes for use, unlike stm32 bitbanding some shifting is still needed. */
  unsigned &pin() const {
    return *reinterpret_cast<unsigned *>(pinn);
  }

  void setRegister(unsigned offset)const{
    *reinterpret_cast<unsigned*>(ctrl+offset) |=  mask;
  }

  inline void clearRegister(unsigned offset)const{
    *reinterpret_cast<unsigned*>(ctrl+offset) &= ~mask;
  }

  inline void assignRegister(unsigned offset,bool level)const{
    if(level){
      setRegister(offset);
    } else {
      clearRegister(offset);
    }
  }

  void setDirection(bool asOutput)const{
    assignRegister(0,asOutput); //the LPC CMSIS code checked before setting, without any explanation as to why that would be needed.
  }

public:

  /** set associated IOCON register to @param pattern.
     * Each pin has its own rules as to what the pattern means, although there are a large set of common patterns. */
  void setIocon(u8 pattern)const {
    reinterpret_cast<unsigned *>(LPC::apb0Device(17))[mode] = pattern + doa;
  }

  /** only special pins should use this directly. */
  inline PortPin(){
    //do not reconfigure on construction, extensions can do that.
  }

  /** read the pin as if it were a boolean variable. */
  inline operator bool() const {
    return PortPin<portNum, bitPosition>::pin() != 0; // need to check assembler, a shift might be better.
  }  

};


/** simple digital input */
template <PortNumber portNum, BitNumber bitPosition> class InputPin: public PortPin<portNum, bitPosition> {
  typedef  PortPin<portNum, bitPosition> super;
  using PortPin<portNum, bitPosition>::clearRegister;
  using PortPin<portNum, bitPosition>::setRegister;
  using PortPin<portNum, bitPosition>::assignRegister;

private:
  bool operator =(bool)const {return false;} // private because this is a read-only entity.

public:
  /** @param yanker controls pullup modality */
  InputPin(): PortPin<portNum, bitPosition>(){
    super::setDirection(0);//gratuitous most of the time.
  }

  void irqAcknowledge() const {
    setRegister(0x1c);
  }

  void irq(bool enable)const{
    assignRegister(16,enable);
  }

  void setIrqStyle(IrqStyle style, bool andEnable)const{
    //disable before recongifuring
    clearRegister(16);

    //  atAddress(regbase)&=~mask; //force to input, user can make it an output and interrupt themselves later if they so wish.
    switch(style){
    case NotAnInterrupt : // in case someone forgets to explicitly select a mode, or we wish to dynamically deconfigure.
      //nothing to do, we have disabled it above so it doesn't matter if we leave it somewhat configured.
      break;
    case AnyEdge: // edge, either edge, input mode buslatch
      clearRegister(4); //pro forma clear of level sense
      setRegister(8);  //double edge
      clearRegister(12);//pro forma clear of direction
      break;
    case LowActive: // level, pulled up
      setRegister(4);
      clearRegister(8);
      clearRegister(12);
      break;
    case HighActive: // level, pulled down
      setRegister(4);
      clearRegister(8);
      setRegister(12);
      break;
    case LowEdge: // edge, pulled up
      clearRegister(4);
      clearRegister(8);
      clearRegister(12);
      break;
    case HighEdge:// edge, pulled down
      clearRegister(4);
      clearRegister(8);
      setRegister(12);
      break;
    }

    if(andEnable){
      setRegister(16);
    }
  }

};

/** configure digital output */
template <PortNumber portNum, BitNumber bitPosition> class OutputPin: public PortPin<portNum, bitPosition>{
  typedef  PortPin<portNum, bitPosition> super;
public:
  /** @param yanker controls pull-up modality */
  OutputPin(): PortPin<portNum, bitPosition>(){
    super::setIocon(digitalPattern(super::doa));
    super::setDirection(1);
  }

  bool operator =(bool newvalue)const{
    super::pin() = newvalue ? ~0 : 0; // don't need to mask or shift, just present all ones or all zeroes and let the hardware 'mask with address' take care of business.
    return newvalue;
  }

};

/** Multiple contiguous bits in a register.
 * This class expedites access using the gpio port 'masked[]' based access
 */
template <PortNumber portNum, unsigned msb, unsigned lsb> class PortField {
  enum {
    width = msb-lsb+1,
    // read the lpc manual, certain address bits are used as a mask
    address = portBase(portNum) | bitMask(lsb+2,width)
  };

public:
  // read
  inline operator unsigned() const {
    return *atAddress(address) >> lsb;
  }
  // write
  inline void operator =(unsigned value) const {
    *atAddress(address) = value << lsb;
  }
  void configurePins(unsigned pattern){
    unsigned pini= pinIndex(portNum, lsb);
    for(unsigned count=width;count-->0;){
      GPIO::setIocon(pini++,pattern);
    }
  }

};

} // namespace LPC
