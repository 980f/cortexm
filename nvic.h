#pragma once

#include "eztypes.h"
#include "peripheraltypes.h"
//not stable part of gcc it appears, will use ours instead. #include "atomic"

//macro's for generating numbers don't work in the irqnumber slot below. The argument must be a simple digit string, no math or lookups or even constexpr's
#define IrqName(irqnumber) IRQ ## irqnumber

//use this in front of the block statement of an irq handler:  (added trailing semicolon to this macro ~2020sep14)
#define HandleInterrupt(irqname)  void IrqName( irqname ) (void);

//see uses, this is a C preprocessor trick to get the symbolic name of an irq to become a number in a timely fashion.
#define ResolveIrq(Irqsymbol)  Irqsymbol

//object based interrupt handlers need some glue:
#define ObjectInterrupt(objCall, irqnumber) HandleInterrupt(irqnumber){ objCall; }

#define FaultName(faultIndex) FAULT ## faultIndex
#define FaultHandler(name, faultIndex) void name(void) __attribute__((alias("FAULT" # faultIndex)))

#define HandleFault(faultIndex) void FaultName(faultIndex) (void)

/** @return previous setting while inserting new one.
the exception number for an irq is irqnumber+16!
*/
u8 setInterruptPriorityFor(unsigned exceptionnumber, u8 newvalue);

///** e.g. SysTick to lowest: setFaultHandlerPriority(15,255);*/
//u8 setFaultHandlerPriority(unsigned faultIndex, u8 level);

extern "C" void disableInterrupt(unsigned irqnum);

/** #of levels for grouping priorities, max 7
 * <sup>7-code</sup> is what actually goes into the hardare register (==~code)
 * stm32F10x only implements the 4 msbs of the logic so values 3,2,1 are same as 0*/
void configurePriorityGrouping(unsigned code);

/** Controls for an irq, which involves bit picking in a block of 32 bit registers.
 * all internals are const so you may use const on every instance, helps the compiler optimize access.
 templated version was too difficult to manage for the slight potential gain in efficiency, i.e. templating was creeping through classes that didn't template well and a base class that the template can extend adds greater runtime overhead than the template can remove. Instead we just inline the code as a template would have required of us and let the compiler work at optimizing this. */
class Irq {
public:

  /** unhandled interrupt handler needed random access by number */
  static constexpr unsigned biasFor(unsigned number) {
    return 0xE000E000 + ((number >> 5) << 2); // NOLINT(hicpp-signed-bitwise)
  }

  static constexpr unsigned bitFor(unsigned number) {
    return number & bitMask(0, 5);
  }
  /** which member of group of 32 this is */
  const unsigned bit;
  /** memory offset for which group of 32 this is in, including the nvic base address */
  const unsigned bias;
  /** bit pattern to go with @see bit index, for anding or oring into 32 bit grouped registers blah blah.*/
  const unsigned mask;
  /** the raw interrupt number */
  const unsigned number;

  explicit Irq(unsigned number) :
      bit(bitFor(number)), bias(biasFor(number)), mask(bitMask(bit)), number(number) {
    //#done
  }

protected:

  /** @returns reference to word related to the feature. @param grup is the offset from the Cortex manuals as published by STM */
  ControlWord controlWord(unsigned grup) const {
    return ControlWord(grup + bias);
  }

  /** this is for the registers where you write a 1 to a bit to make something happen. */
  void strobe(unsigned grup) const {
    controlWord(grup) = mask;
  }

public:
  /** @returns the state of the group for this interrupt, the most interesting groups have dedicated functions here.  */
  bool irqflag(unsigned grup) const {
    return (mask & controlWord(grup)) != 0;
  }

  u8 setPriority(u8 newvalue) const {
    return setInterruptPriorityFor(number+16, newvalue);//seems like this was wrong for many years due to lack of +16.
  }

  /** @returns whether the source of the request is active */
  bool isActive() const {
    return irqflag(0x300);
  }

  /** @returns whether the individual enable is active */
  bool isEnabled() const {
    return irqflag(0x100);
  }

  void enable() const {
    strobe(0x100);
  }
  /** simulate the interrupt, expect it to be handled before the next line of your code. */
  void fake() const {
    strobe(0x200);
  }

  void clear() const {
    strobe(0x280);
  }

  void disable() const {
    strobe(0x180);
  }

  /** for some devices you must acknowledge a prior interrupt before enabling */
  void prepare() const {
    clear();
    enable();
  }

  /** enable or disable */
  void operator=(bool on) const { // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
    if (on) {
      enable();
    } else {
      disable();
    }
  }

  /** @returns whether the interrupt is enabled, NOT the state of the request. */
  operator bool() const { // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    return isEnabled();
  }
};

#include "core-atomic.h"

/** tool for managing disabling an interrupt in a nesting fashion, ie a function that needs to disable the interrupt can call another
 * function that also needs to disable the interrupt and this guy if properly used can ensure that the interrupt isn't reenabled until the first disabler wished to do so.
 * See IRQLock for the safest way to use this class.
 * instantiating more than one of these for a given interrupt defeats the nesting nature of its enable.
 */
class GatedIrq: public Irq {
  unsigned locker; //tracking nested attempts to lock out the interrupt.
public:
  explicit GatedIrq(unsigned number) :
      Irq(number), locker(0) {
  }

  void enable() {
    if (atomic_decrementNowZero(locker)) {
      enable();
      // if locked then reduce the lock such that the unlock will cause an enable
      // one level earlier than it would have. This might be surprising so an
      // unmatched unlock might be the best enable.(so we renamed this method to 'enable' ;)
    }
  }

  void lock() {
    if (atomic_incrementWasZero(locker)) {
      disable();
    }
  }

  void prepare() {
    clear(); // acknowledge to hardware
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
struct IRQLock {
  GatedIrq &irq;
public:
  explicit IRQLock(GatedIrq &irq) :
      irq(irq) {
    irq.lock();
  }

  ~IRQLock() {
    irq.enable();
  }
};

#ifdef __linux__ //just compiling for syntax checking
extern bool IrqEnable;
#define IRQLOCK(irq)

#else
#include "core_cmFunc.h"
#define IRQLOCK(irqVarb) IRQLock IRQLCK ## irqVarb(irqVarb)
#endif

//this does not allow for static locking, only for within a function's execution (which is a good thing!):
#define LOCK(somename) CriticalSection somename ## _locker

/** creating one of these in a function (or blockscope) disables <em> all </em> interrupts until said function (or blockscope) exits.
 * By using this fanciness you can't accidentally leave interrupts disabled. */
class CriticalSection {
  static volatile unsigned nesting;
public:
  CriticalSection() {
    if (!nesting++) {
      IrqEnable = false;
    }
  }

  ~CriticalSection() {
    if (nesting) { //then interrupts are globally disabled
      if (--nesting == 0) {
        IrqEnable = true;
      }
    }
  }
};

