#pragma once

#include "eztypes.h"
#include "peripheraltypes.h"

//macro's for generating numbers don't work in the irqnumber slot below. The argument must be a simple digit string, no math or lookups or even constexpr's
#define IrqName(irqnumber) IRQ ## irqnumber

//use this in front of the block statement of an irq handler:
#define HandleInterrupt(irqname)  void IrqName( irqname ) (void)

//object based interrupt handlers need some glue:
#define ObjectInterrupt(objCall, irqnumber) HandleInterrupt(irqnumber){ objCall; }


#define FaultName(faultIndex) FAULT ## faultIndex
#define FaultHandler(name, faultIndex) void name(void) __attribute__((alias("FAULT" # faultIndex)))

#define HandleFault(faultIndex) void FaultName(faultIndex) (void)

/** @return previous setting while inserting new one*/
u8 setInterruptPriorityFor(unsigned irqnum, u8 newvalue);

/** e.g. SysTick to lowest: setFaultHandlerPriority(15,255);*/
void setFaultHandlerPriority(int faultIndex, u8 level);

extern "C" void disableInterrupt(unsigned irqnum);

/** value to put into PRIGROUP field, see arm tech ref manual.
  * 0: maximum nesting; 7: totally flat; 2<sup>7-code</sup> is number of different levels
  * stm32F10x only implements the 4 msbs of the logic so values 3,2,1 are same as 0*/
void configurePriorityGrouping(int code);

/** unhandled interrupt handler needed random access by number, so we constexpr'd the pieces out of the template class */


constexpr unsigned biasFor(unsigned number){
  return 0xE000E000 + ((number>>5)<<2);
}

constexpr unsigned bitFor(unsigned number){
  return number & bitMask(0,5);
}

/** Controls for an irq, which involves bit picking in a block of 32 bit registers */
template <unsigned number> class Irq {
public:
  enum {
    /** which member of group of 32 this is */
    bit=bitFor(number),
    /** memory offset for which group of 32 this is in */
    bias=biasFor(number),
    /** bit pattern to go with @see bit index, for anding or oring into 32 bit grouped registers blah blah.*/
    mask=bitMask(bit)
  };

protected:

  /** @returns reference to word related to the feature. */
  inline constexpr unsigned &controlWord(unsigned grup)const{
    return *atAddress(grup | bias);
  }

  /** this is for the registers where you write a 1 to a bit to make something happen. */
  inline void strobe(unsigned grup)const{
    controlWord(grup) = mask;
  }

public:

  bool irqflag(unsigned grup)const{
    return (mask & controlWord(grup))!=0;
  }

  u8 setPriority(u8 newvalue) const {
    return setInterruptPriorityFor(number,newvalue);
  }


  bool isActive(void) const {
    return irqflag(0x300);
  }

  bool isEnabled(void) const {
    return irqflag(0x100);
  }

  void enable(void) const {
    strobe(0x100);
  }

  void fake(void) const {
    strobe(0x200);
  }

  void clear(void) const {
    strobe(0x280);
  }

  void disable(void) const {
    strobe(0x180);
  }

  void prepare() const {
    clear();
    enable();
  }

  void operator=(bool on) const {
    if(on){
      enable();
    } else {
      disable();
    }
  }

  operator bool() const {
    return isEnabled();
  }
};

#include "core-atomic.h"

/** tool for managing disabling an interrupt in a nesting fashion, ie a function that needs to disable the interrupt can call another
 * function that also needs to disable the interrupt and this guy ifproperly used can ensure that the interrupt isn't reenabled until the first disabler wished to do so.
 * See IRQLock for the safest way to use this class.
 * instantiating more than one of these for a given interrupt defeats the nesting nature of its enable. */
template <unsigned number> class GatedIrq: public Irq<number> {
  int locker; //tracking nested attempts to lock out the interrupt.
public:
  GatedIrq():locker(0){}

  void enable(void){
    if(locker > 0) { // if locked then reduce the lock such that the unlock will cause an enable
      --locker;  // one level earlier than it would have. This might be surprising so an
      // unmatched unlock might be the best enable.(so we renamed this method to 'enable' ;)
    }
    if(locker == 0) { // if not locked then actually enable
      Irq<number>::enable();
    }
  }

  void lock(){
    if(locker++ == 0) {
      Irq<number>::disable();
    }
  }

  void prepare(){
    Irq<number>::clear(); // acknowledge to hardware
    enable(); // allow again
  }

};

/** disable interrupt on creation of object, enable it on destruction
  * usage: create one within a block where the irq must not be honored.
  * note: this cheap implementation may turn on an interrupt that was off,
  *  don't lock if you can't tolerate the interrupt being enabled.
  *  Since each interrupt can be stifled at its source this should not be a problem.
  *  future: automate detection of being in the irq service and drop the argument.
  */
template <unsigned number> struct IRQLock {
  static GatedIrq<number> irq;
public:
  IRQLock(){
    irq.lock();
  }

  ~IRQLock(){
    irq.enable();
  }
};


#ifdef __linux__ //just compiling for syntax checking
extern bool IRQEN;
#define IRQLOCK(irq)

#else
#include "core_cmFunc.h"
extern const CPSI_i IRQEN;
#define IRQLOCK(irqVarb) IRQLock IRQLCK ## irqVarb(irqVarb)
#endif

//this does not allow for static locking, only for within a function's execution (which is a good thing!):
#define LOCK(somename) CriticalSection somename ## _locker

/** creating one of these in a function (or blockscope) disables all interrupts until said function (or blockscope) exits.
  * By using this fanciness you can't accidentally leave interrupts disabled. */
class CriticalSection {
  static volatile unsigned nesting;
public:
  CriticalSection(void){
    if(!nesting++){
      IRQEN=0;
    }
  }

  ~CriticalSection (void){
    if(nesting) { //then interrupts are globally disabled
      if(--nesting == 0) {
        IRQEN=1;
      }
    }
  }
};

