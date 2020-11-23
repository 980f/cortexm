#pragma once

#include "eztypes.h"
#include "peripheraltypes.h"

/** Instead of having a long list of particularly spelled interrupt handler names and also a list of interrupt number names we can get by with just
 * the number names and generate handler names from them.
 *
 * The IrqName and HandleInterrupt macros below are used in application code, and you can make macros like UartIrq(uartNumber) and use those
 * both in the interrupt controls defined here (class Irq) and to mark the handler.
 *
 * Most interrupt handlers are oneliners that call a shared function with an arugment for the instance of the peripheral.
 *
 * #define myIrqNum 29
 * const Irq myIrq(myIrqNum);
 * ...
 * myIrq.setPriority(...);
 * ...
 * HandleInterrupt(myIrqNum){
 *   myObject.handle();
 * }
 *
 * ******************************
 *
 * The following do the same for faults:
 * FaultName(faultIndex) MACRO_cat(FAULT , faultIndex)
 * FaultHandler(name, faultIndex)
 * HandleFault(faultIndex)
 *
 * ******************************
 *
 * This allows us to build the vector table with names that are independent of the peripherals, which means that they are also
 * independent of the vendor other than for the highest valued interrupt that can occur.
 * I weak define all of them to a common "unhandled interrupt handler".
 * My version of that handler disables the interrupt, using the NVIC registers to figure that out.
 * You may wish to put a halt there instead of a simple disable and return.
 *
 * That takes up less bytes of rom than the usual copy of "branch here" for each handler.
 * */

/**irqnumber can be a decimal or a macro that resolves to one. To ensure proper resolution of macro to number see MACRO_wrap and frineds in eztypes.h.
 you will detect a failure by your interrupt handler not gtting called despite interrupt being taken, a breakpoint in */
#define IrqName(irqnumber) MACRO_cat(IRQ , irqnumber)


/** use the following macro in front of the open curly of the function body of an irq handler:
 HandleInterrupt(numberish){
   isrcode();
 }
*/
#define HandleInterrupt(irqname)  void IrqName( irqname ) (void)

#define FaultName(faultIndex) MACRO_cat(FAULT , faultIndex)
#define FaultHandler(name, faultIndex) void name(void) __attribute__((alias("FAULT" # faultIndex)))

#define HandleFault(faultIndex) void FaultName(faultIndex) (void)

//there are 16 possible faults, address space is always reserved for them whether the particular chip has the fault or not.
constexpr unsigned FaultBias=16;

/** @return previous setting while inserting new one.
 * give this 0..15, we will put that where it belongs.
the exception number for an irq is irqnumber, for fault a negative value, -1 to -15
*/
u8 setInterruptPriorityFor(int exceptionnumber, u8 newvalue);

extern "C" void disableInterrupt(unsigned irqnum);

/** #of levels for grouping priorities, max 7
 * <sup>7-code</sup> is what actually goes into the hardware register (==~code)
 * stm32F10x et al. only implements the 4 msbs of the logic so values 3,2,1 are same as 0 */
void configurePriorityGrouping(unsigned code);

/** Controls for an irq, which involves bit picking in a block of 32 bit registers.
 * all internals are const so you may use const on every instance, helps the compiler optimize access.
 templated version was too difficult to manage for the slight potential gain in efficiency, i.e. templating was creeping through classes that didn't template well and a base class that the template can extend adds greater runtime overhead than the template can remove. Instead we just inline the code as a template would have required of us and let the compiler work at optimizing this. */
class Irq {
public:
  static void setAllPriorties(u8 prio);
  /** unhandled interrupt handler needed random access by number */
  static constexpr unsigned biasFor(unsigned number) {
    return 0xE000E000 + ((number >> 5) << 2);
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

  explicit constexpr Irq(unsigned number) :
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
    return setInterruptPriorityFor(number, newvalue);
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
  void operator=(
    bool on) const { // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
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
class GatedIrq : public Irq {
  unsigned locker; //tracking nested attempts to lock out the interrupt.
public:
  explicit GatedIrq(unsigned number) :
    Irq(number), locker(0) {
  }

  void enable() {
    if (atomic_decrementNowZero(locker)) {//todo:000 seems to be defective!
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

/** see IRQLOCK macro for proper instatiation of one of these.
 * disable interrupt on creation of object, enable it on destruction
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

/** simplest locker, always disables, always enables. */
struct IRQblock {
  const Irq &irq;

  IRQblock(const Irq &irq) : irq(irq) {
    irq.disable();
  }

  ~IRQblock() {
    irq.enable();
  }
};

/** simple locker, always disables, enables if was enabled.
 * This cannot deal with some other interrupt service routine deciding that the state of this interrupt should change
 * while this guy is running. That is weird enough to not worry about, since most usages only gate the interrupt for
 * making access to a variable shared by the isr and foreground atomic.
 * Any strange gating as in this edge case should be done on the upstream enable in the peripheral itself, not here in the NVIC.
 * */
struct IRQstacker {
  const Irq &irq;
  const bool wasEnabled;

  IRQstacker(const Irq &irq) : irq(irq), wasEnabled(irq.isEnabled()) {
    irq.disable();
  }

  IRQstacker(const Irq &irq,bool inISR) : irq(irq), wasEnabled(!inISR && irq.isEnabled()) {
    if(!inISR) {
      irq.disable();
    }
  }

  ~IRQstacker() {
    if (wasEnabled) {
      irq.enable();
    }
  }
};


#ifdef __linux__ //just compiling for syntax checking
extern bool IrqEnable;
#define IRQLOCK(irq)

#else

#include "core_cmFunc.h"

//not working quite right, try IRQBLOCK or IRQSTACK until we remove this comment.
#define IRQLOCK(irqVarb) IRQLock IRQLCK ## irqVarb(irqVarb)
//unconditionally disable, then enable on scope exit
#define IRQBLOCK(irqVarb) IRQblock IRQBLCK ## irqVarb(irqVarb)
//use this when the interrupt might not be on
#define IRQSTACK(irqVarb,...) IRQstacker IRQPUSH ## irqVarb(irqVarb , __VA_ARGS__)

#endif

//this does not allow for static locking, only for within a function's execution (which is a good thing!):
#define LOCK(somename) CriticalSection somename ## _locker

/** creating one of these in a function (or blockscope) disables <em> all </em> interrupts until said function (or blockscope) exits.
 * By using this fanciness you can't accidentally leave interrupts disabled. */
class CriticalSection {
  /** Note Well the static below, that is essential for this guy to work. */
  static volatile unsigned nesting;//zero init by C startup.
public:
  CriticalSection() {
    if (!nesting++) {
      IrqEnable = false;
    }
  }

  ~CriticalSection() {
    if (nesting) { //this should always be non-zero, someone has to attack this well hidden variable from outside the class,
      //... but testing it before decrementing it is free with even modest optimization turned on, it has to be loaded into a register to be decremented.
      if (--nesting == 0) {
        IrqEnable = true;
      }
    }
  }
};

