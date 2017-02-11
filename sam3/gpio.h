#ifndef GPIO_H
#define GPIO_H

#include "peripheraltypes.h"
#include "feature.h"
#include "samperipheral.h"

enum Port {
  PA=0,
  PB,
  PC,
  //sam u stops here, sam x continues
  PD,
  //commercially available samX stops here as far as pins go.
  PE,
  PF,
};

constexpr unsigned portBase(unsigned portnum){
  return scmbase(portnum+8);//6 for samU
}

/** declared outside of xxPin class so that we don't have to apply template args to each use.*/
enum PinBias {
  LeaveFloating = 0, // in case someone forgets to explicitly select a mode
  PullUp, // level, pulled up
  Bussed, //opendrain output

};

/** register groups, a private enum */
enum GpioFeature {
  BeSimple=0,
  BeOutput,
  Glitcher,
  Data, // set 1's, set 0's, data port is 32 bit writable as well as bit mask readable., 4th group is actual pin level
  InterruptMask,//4: interrupt, usual triplet, 4th member is 'interrupt latched', clears on read!:)
  OpenDrain, //5: open collector/drain enable
  NoPullup, //6: *disable* pullup
  WhichPeripheral, //7: B-peripheral, else A (see also feature 0)
  GlitchRate, //8: glitch filter clock select, on=mck, else div_slck, see feature 2. manual implies opposite polarity on readback. 4th member is slow clock divider.
  //... Tdiv_slclk = 2*(DIV+1)*Tslow_clock
  NothingAt9, //9: no use
  LockOutputState, //10: lock output state
  PolarInterrupt, //11: additional interrupt mode(0=both edge per feature 4), no readback
  EdgyInterrupt, //12: edge else level, inverted readback?
  InterruptPolarity, //13: low edge/low active, inverted readback?
  ControlLock, //14: isLocked, writeprotect gizmo., writeprotect faulted (clear on read) (no dox on how to map 16 bit code to which register was attacked)
};

#ifndef ARDUINO
Handler &gpioVector(unsigned portnum,unsigned pinnumber);
/** call all the gpioVector functions whose corresponding bits are set */
void hike(unsigned portnum,unsigned bits);
#endif

/** those settings that are shared across a port */
template <unsigned portnum> class PIOPort {
  enum {
    base=portBase(portnum), /** unlike stm the base is not derivable from the pid */
    pid=11+portnum,  /** used for clock enable and irq computation. */  //10 for samU
  };
public:

  void enable(bool clockOn)const{
    //clock system
  }
  /** @param on when true locks out spurious writes to the control register */
  void configurationLock(bool on)const{
  }
  /* set glitch filter options for the bank, @see also */
  void setGlitchFilterBase(unsigned slowrate)const{
    *atAddress(base+0x8C)=slowrate;
  }

  /** read interrupt flag register, bit per pin, read resets flags so this must be done at group level.
    do NOT do this on an Arduino unless you have overridden the PIO interrupt handler  */
  unsigned getInterrupts()const{
    return *atAddress(base+0x4C);
  }
#ifndef ARDUINO  //we don't have the info here needed to call the Arduino interrupt attachment function
  void setHandler(unsigned pinnumber, Handler handler)const{
    gpioVector(portnum,pinnumber)=handler;
  }
#endif

};

/** to configure a pin for a dedicated function one must merely construct a GpioPin with template args for which pin and constructor arg of control pattern*/
template <unsigned portNum, unsigned bitPosition> class PortPin: public BoolishRef {
  typedef PortPin<portNum,bitPosition> Base;
protected: // for simple gpio you must use an extended class that defines read vs read-write capability.
  enum {
    mask = 1 << bitPosition, // used for port control register access
    base = portBase(portNum), // base for port control
  };

  const SAM::Feature feature(GpioFeature which,bool inverted=false)const{
    return SAM::Feature(base+(which<<4),mask,inverted);
  }

  inline void setRegister(GpioFeature which)const{
    feature(which)=1;
  }

  inline void clearRegister(GpioFeature which)const{
    feature(which)=0;
  }

  inline void assignRegister(GpioFeature which,bool level)const{
    feature(which)=level;
  }

  void setDirection(bool asOutput)const{
    assignRegister(BeOutput,asOutput);
  }

protected:
  /** 0==just a pin,  1: selection A, 2==selection B, rtfm.*/
  void setFunction(unsigned noAB){
    assignRegister(BeSimple,noAB==0);
    assignRegister(WhichPeripheral,noAB==2);
  }

  /** read the pin as if it were a boolean variable. */
  inline operator bool() const {
    SAM::Feature arf=feature(Data); // need to check assembler, a shift might be better.
    return arf;
  }

};


/** simple digital input */
template <unsigned portNum, unsigned bitPosition> class InputPin: public PortPin<portNum, bitPosition> {
  typedef  PortPin<portNum, bitPosition> Base;

private:
  bool operator =(bool)const {return false;} // private because this is a read-only entity.

public:
  /** @param yanker controls pullup modality */
  InputPin(PinBias yanker = PullUp): PortPin<portNum, bitPosition>(){
    //todo: set direction to 0, which is the power up setting so not urgent in our typical use of static configuration.
    PortPin<portNum, bitPosition>::setDirection(0);
  }
#ifndef ARDUINO  //@see interruptPin class
  /** enable within the group, still will need to nvic enable the group and scan bits in the grouped status register. */
  void irq(bool enable)const{
    Base::feature(InterruptMask) = enable;
  }

  void setIrqStyle(IrqStyle style, bool andEnable)const{
    Base::feature(PolarInterrupt) = style>AnyEdge;

    Base::feature(EdgyInterrupt) =     style==LowEdge || style==HighEdge;
    Base::feature(InterruptPolarity) = style==HighActive|| style==HighEdge;

    if(andEnable){
      irq( style>=AnyEdge );
    }
  }
#endif

};

/** simple digital output */
template <unsigned portNum, unsigned bitPosition> class OutputPin: public PortPin<portNum, bitPosition>{
  typedef PortPin<portNum, bitPosition> Base;
public:
  /** @param yanker controls pull-up modality */
  OutputPin(PinBias yanker = Bussed): PortPin<portNum, bitPosition>(){
    Base::setDirection(1);
    Base::feature(OpenDrain) = yanker==Bussed;
  }

  bool operator =(bool newvalue)const{
    Base::feature(Data) = newvalue;
    return newvalue;
  }

  /** enabling the lock means that further setting of the pin is ignored */
  void lock(bool enable)const {
    Base::feature(LockOutputState)=enable;
  }

};

///** Multiple contiguous bits in a register.
// * This class expedites access using the gpio port 'masked[]' based access
// */
//template <unsigned portNum, unsigned msb, unsigned lsb> class PortField {
//  enum {
//    // read the lpc manual, certain address bits are used as a mask
//    address = portBase(portNum),
//    mask= bitMask(lsb,msb-lsb+1)
//  };

//public:
//  // read
//  inline operator unsigned() const {
//    return *atAddress(address) >> lsb;
//  }
//  // write
//  inline void operator =(unsigned value) const {
//    *atAddress(address) = value << lsb;
//  }
//  void configurePins(unsigned pattern){
////    unsigned pini= pinIndex(portNum, lsb);
////    for(unsigned which=lsb;which++<=msb;){
////      GPIO::setIocon(pini++,pattern);
////    }
//  }

//};


#endif // GPIO_H
