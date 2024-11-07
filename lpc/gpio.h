#pragma once

#include "lpcperipheral.h"
#include "cheaptricks.h"
#include "boolish.h"
#include "bitbasher.h"

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
constexpr unsigned ioconf_map[] =
{ // pass this a pinIndex
  3, 4, 7, 11, 12, 13, 19, 20, 24, 25, 26, 29,
  30, 31, 32, 36, 37, 40, 41, 42, 5, 14, 27, 38,
  2, 10, 23, 35, 16, 17, 0, 8, 9, 21, 22, 28,
  33, 34, 39, 43, 15, 18,
  // 1 6 are not present in this list (reserved locations in hardware map), gok what they may be used for someday.
};


/** the pins for which this are true have different io configuration patterns than the rest.
// 0.0 0.10 0.11   1.0 1.1 1.2 1.3 */
constexpr bool isDoa(unsigned pinIndex){
  return (15>= pinIndex && pinIndex >=10)||pinIndex==0;
}

constexpr unsigned gpioBankInterrupt(PortNumber portNum){
  return 56-portNum;
}

/** declared outside of InputPIn class so that we don't have to apply template args to each use.*/
enum PinBias { //#ordered for MODE field of iocon register
  LeaveFloating = 0, // in case someone forgets to explicitly select a mode
  PullDown, // level, pulled down
  PullUp, // level, pulled up
  BusLatch, // edge, either edge, input mode buslatch
};


/** express access to a pin.
 * will add field access objects when that proves useful.
 */
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

  void setIocon(unsigned pattern)const{
    GPIO::setIocon(pini,pattern);
  }

public:
  /** this must be called once before any other functions herein are used. Declaring it to be in an init section is a nice way to guarantee that.
As of 2017jan14 SystemInit is calling this, that is simpler to maintain than an array of function pointers in an explicit section.
*/
  static void Init( void );

  /** biasing is independent of in vs out, but not of function. */
  static constexpr unsigned ioconPattern(PinBias bias){
    // FYI P0.4 and P0.5 reset to 0, all others to D0
    //  2 bits are the code passed in
    //   ls 3 bits are either 0 for normal pins or 1 for doa pins like reset or SWD pins.
    return (1<<7) | (1 << 6) | (bias << 3);
    // ignoring hysteresis option, it depends upon VDD so we'd have to define a symbol for that.
  }

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


  static constexpr unsigned analogInputPattern(){
    // we disable pullups and pulldowns on analog channels (bias==0)
    // bit 7 is a zero for analog selection
    return (1 << 6) | 1;
  }

  /** set the ioconfiguration for the given pin to the given pattern.
   the pattern is adjusted herein for @see isDoa pins */
  static void setIocon(unsigned pinIndex,unsigned pattern){
    //for those patterns we have generators for doa pins need a one added to them.
    reinterpret_cast<unsigned *>(LPC::apb0Device(17))[ioconf_map[pinIndex]]=pattern + isDoa(pinIndex);
  }

  void setDirection(bool output)const;
  void setIrqStyle(IrqStyle style,bool andEnable)const;
  void irq(bool enable)const;
  /** clear pending bit*/
  void irqAcknowledge()const;


};



class Output: public GPIO {
public:
  Output(PortNumber portNum, BitNumber bitPosition):GPIO(portNum,bitPosition){
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
  void configurePins(unsigned pattern)const{
    unsigned pinIndex= lsb+ ((address>>16)&3)*12;
    u16 picker=1<<(lsb+2);
    while(address & picker){
      GPIO::setIocon(pinIndex++,pattern);
      picker<<=1;
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
    setDirection(true);
  }
  /** write data to pins, but only effective if an output, doesn''t cause a spontaneous reconfiguration */
  inline unsigned operator =(unsigned value) const {
    *atAddress(address) = value << lsb;
    return value;
  }
};

// and now for the modern approach:
/** to configure a pin for a dedicated function one must merely construct a GpioPin with template args for which pin and constructor arg of control pattern*/
template <PortNumber portNum, BitNumber bitPosition> class PortPin: public BoolishRef {
public:
  enum {
    pini = pinIndex(portNum, bitPosition)
  };

protected: // for simple gpio you must use an extended class that defines read vs read-write capability.
  enum {
    mask = 1 << bitPosition, // used for port control register access
    base = portBase(portNum), // base for port control
    // some pins are special on reset
    doa = (portNum == 0 && (bitPosition == 0 || bitPosition == 10 || bitPosition == 11)) || // 0.0 0.10 0.11
    (portNum == 1 && (bitPosition < 4)), // 1.0 1.1 1.2 1.3
    mode = ioconf_map[pini], // iocon array index
    pinn = base + (mask << 2),    // physical pin 'masked' access location "address == pattern"
    ctrl = base + (1<<15)
  };

  /** set associated IOCON register to @param pattern.
     * Each pin has its own rules as to what the pattern means, although there are a large set of common patterns. */
  inline void setIocon(unsigned pattern){
    GPIO::setIocon(pini,pattern);
  }

  /** @returns reference to the masked access port of the register, mask set to the one bit for this pin. @see InputPin and @see OutputPin classes for use, unlike stm32 bitbanding some shifting is still needed. */
  inline unsigned &pin() const {
    return *reinterpret_cast<unsigned *>(/*PortPin<portNum, bitPosition>::*/pinn);
  }

  inline void setRegister(unsigned offset)const{
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

  /** only special pins should use this directly. */
  inline PortPin(unsigned pattern){
    setIocon(pattern);
  }

  // biasing is independent of in vs out, but not of function.
  static constexpr unsigned ioconPattern(PinBias bias){
    // FYI P0.4 and P0.5 reset to 0, all others to D0
    //  2 bits are the code passed in
    //   ls 3 bits are either 0 for normal pins or 1 for doa pins like reset or SWD pins.
    return (1<<7) | (1 << 6) | (bias << 3) | doa;
    // ignoring hysteresis option, it depends upon VDD so we'd have to define a symbol for that.
  }

  static constexpr unsigned analogInputPattern(){
    // the doa pins get a 2 rather than a 1 to select analog functionality
    // we disable pullups and pulldowns on analog channels (bias==0)
    // bit 7 is a zero for analog selection
    return (1 << 6) | (doa?2:1);
  }

  /** read the pin as if it were a boolean variable. */
  inline operator bool() const {
    return PortPin<portNum, bitPosition>::pin() != 0; // need to check assembler, a shift might be better.
  }

};


/** simple digital input */
template <PortNumber portNum, BitNumber bitPosition> class InputPin: public PortPin<portNum, bitPosition> {
  using PortPin<portNum, bitPosition>::clearRegister;
  using PortPin<portNum, bitPosition>::setRegister;
  using PortPin<portNum, bitPosition>::assignRegister;

private:
  bool operator =(bool)const {return false;} // private because this is a read-only entity.

public:
  /** @param yanker controls pullup modality */
  InputPin(PinBias yanker = BusLatch): PortPin<portNum, bitPosition>(this->ioconPattern(yanker)){
    //todo: set direction to 0, which is the power up setting so not urgent in our typical use of static configuration.
    PortPin<portNum, bitPosition>::setDirection(0);
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

/** simple digital output */
template <PortNumber portNum, BitNumber bitPosition> class OutputPin: public PortPin<portNum, bitPosition>{
  typedef  PortPin<portNum, bitPosition> super;
public:
  /** @param yanker controls pull-up modality */
  OutputPin(PinBias yanker = BusLatch): PortPin<portNum, bitPosition>(this->ioconPattern(yanker)){
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
    // read the lpc manual, certain address bits are used as a mask
    address = portBase(portNum) | bitMask(lsb+2,msb-lsb+1)
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
    for(unsigned which=lsb;which++<=msb;){
      GPIO::setIocon(pini++,pattern);
    }
  }

};

} // namespace LPC
