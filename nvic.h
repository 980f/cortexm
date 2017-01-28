#pragma once

#include "eztypes.h"


//macro's for generating numbers don't work in the irqnumber slot below. The argument must be a simple digit string, no math or lookups or evenconsexpre's
#define IrqName(irqnumber) IRQ ## irqnumber

//use this in front of the block statement of an irq handler:
#define HandleInterrupt(irqname)  void IrqName( irqname ) (void)

//object based interrupt handlers need some glue:
#define ObjectInterrupt(objCall, irqnumber) HandleInterrupt(irqnumber){ objCall; }


#define FaultName(faultIndex) FAULT ## faultIndex
#define FaultHandler(name, faultIndex) void name(void) __attribute__((alias("FAULT" # faultIndex)))

#define HandleFault(faultIndex) void FaultName(faultIndex) (void)

/** base class to Irq which can be const'ed or even templated. */
class Irq {
public:
  /** official number per your mcu's documents */
  const u8 number;
  /** which member of group of 32 this is */
  const u8 bit;
  /** memory offset for which group of 32 this is in */
  const u32 bias;
  /** bit pattern to go with @see bit index, for anding or oring into 32 bit grouped registers blah blah.*/
  const u32 mask;

  /* using u8 data type to check validity, need an 'explicit' somewhere to make that fully true */
  Irq(u8 number):
    number(number),
    bit(number & 0x1F),
    bias(0xE000E000 + ((number>>5)<<2)),
    mask(1<<(number & 0x1F)){//redo bit equation hoping to get init block instead of init routine
    /*empty*/
  }

protected:

  /** @returns reference to word related to the feature. */
  inline unsigned &controlWord(unsigned grup)const{
    return *reinterpret_cast <unsigned *> (grup | bias);
  }

  /** this is for the registers where you write a 1 to a bit to make something happen. */
  inline void strobe(unsigned grup)const{
    controlWord(grup) = mask;
  }

public:

  bool irqflag(unsigned grup)const{
    return (mask & controlWord(grup))!=0;
  }
  /** @return previous setting while inserting new one*/
  u8 setPriority(u8 newvalue) const;

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

};

/** instantiating more than one of these for a given interrupt defeats the nesting nature of its enable. */
class GatedIrq: public Irq {
  int locker; //tracking nested attempts to lock out the interrupt.
public:
  GatedIrq(int number): Irq(number), locker(0){}

  void enable(void);
  void lock(void);
  /** clear any pending then enable, regular enable will cause an interrupt if one is pending.*/
  void prepare(void);

};

/** disable interrupt on creation of object, enable it on destruction
  * usage: create one within a block where the irq must not be honored.
  * note: this cheap implementation may turn on an interrupt that was off,
  *  don't lock if you can't tolerate the interrupt being enabled.
  *  Since each interrupt can be stifled at its source this should not be a problem.
  *  future: automate detection of being in the irq service and drop the argument.
  */
struct IRQLock {
  GatedIrq&irq;
public:
  IRQLock(GatedIrq&irq, bool inIrq = false);
  ~IRQLock();
};

/** e.g. SysTick to lowest: setFaultHandlerPriority(15,255);*/
void setFaultHandlerPriority(int faultIndex, u8 level);
/** value to put into PRIGROUP field, see arm tech ref manual.
  * 0: maximum nesting; 7: totally flat; 2<sup>7-code</sup> is number of different levels
  * stm32F10x only implements the 4 msbs of the logic so values 3,2,1 are same as 0*/
void configurePriorityGrouping(int code); //cortexm3.s or stm32.cpp

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

/** creating one of these in a function (or blockscope) disables interrupts until said function (or blockscope) exits.
  * By using this fanciness you can't accidentally leave interrupts disabled. */
class CriticalSection {
  static volatile int nesting;
public:
  CriticalSection(void){
    IRQEN=0;
    ++nesting;
  }

  ~CriticalSection (void){
    if(nesting != 0) { //then interrupts are globally disabled
      if(--nesting == 0) {
        IRQEN=1;
      }
    }
  }
};

