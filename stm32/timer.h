#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"
#pragma once

/**
  *
  * All 3 classes of timers have identical register layout, some are just missing parts.
  * Timers 1 and 8 have everything,
  * 2..5 are missing the complementary compare outputs
  * 6&7 only count, no capture/compare
  *
  * This is a comprehensively 16 bit peripheral, only the low half of any 32 bit control word
  * has any functioning.
  *
  * F407: for some timers some registers are 32 bit instead of 16. We can template that point.
  */

#include "stm32.h"
#include "nvic.h"
#include "minimath.h"

#include "gpio.h" //for control of associated pins

//construction aid
struct TimerConstant {
  BusNumber apb;
  u8 slot;
  u8 irq;
//  enum DebugControlBit stopper;
};

#define TimerTableSpacer   {CPU, 0, 0},

static constexpr TimerConstant T[] = {
  TimerTableSpacer  //spacer so that we can textually use st's 1-based labels.
#if DEVICE == 103    //T1..8
  { APB2, 11, 27 }, //CC, see also 25,26 ...

  { APB1, 0, 28 },
  { APB1, 1, 29 },
  { APB1, 2, 30 },
  { APB1, 3, 50 },
  { APB1, 4, 54 },
  { APB1, 5, 55 },

  { APB2, 13, 44 },
#elif DEVICE == 407
//stop bit is apb*32+slot
  {APB2, 0, 25}, //T1 update

  {APB1, 0, 28},
  {APB1, 1, 29}, //T3
  {APB1, 2, 30},
  {APB1, 3, 50},
  {APB1, 4, 54},//shared with DAC
  {APB1, 5, 55},//T7???

  {APB2, 1, 44},//T8 update

  {APB2, 16, 24},//#9 and T1 break
  {APB2, 17, 25},//#10 and T1 update
  {APB2, 18, 26},//#11 and T1 trigger
  {APB1, 6, 43},//#12 and T8 break
  {APB1, 7, 44},//#13 and T8 update
  {APB1, 8, 45},//#14 and T8 trigger
#elif DEVICE == 452  //lots of gaps!
  {APB2, 11, 25}, //T1
  {APB1, 0, 28}, //T2
  {APB1, 1, 29}, //T3
  TimerTableSpacer
  TimerTableSpacer
  {APB1, 4, 54},//T6
  TimerTableSpacer

  TimerTableSpacer
  TimerTableSpacer
  TimerTableSpacer
  TimerTableSpacer
  TimerTableSpacer
  TimerTableSpacer
  TimerTableSpacer
  {APB2, 16, 24},//#15

  {APB2, 17, 25},//#16 overlaps T1

#endif
};

/**
 * stm32 hardware timers
 The variations are simply missing pieces, so don't use them if you don't know you have them.
 Timers 2 and 5 on the F407 have 32 bits in a few places where this class only supports 16, that is not yet accommodated.
 What makes it hard is that the device requires 16 bit accesses when the register is only 16 bits, at least the F103 does.
 * */

struct Timer : public APBdevice {
  const Irq irq;
  const unsigned luno; //handy for debug
  const ControlBit enable;
  const ControlBit UIE;
  const ControlBit UIF;
  const ControlBit OPM;
  const ControlBit UG;
  const ControlWord PSC;
  const ControlWord ARR;

  bool isAdvanced() const {
    return luno == 1 || luno == 8;
  }

  bool is32bit() const {
#if DEVICE == 103
    return false;
#elif DEVICE == 407
    return luno == 2 || luno == 5;
#elif DEVICE == 452
    return luno == 2;
#endif
  }

  constexpr Timer(unsigned stLuno) : APBdevice(T[stLuno].apb, T[stLuno].slot), irq(T[stLuno].irq), luno(stLuno) //logical parts
    , enable(registerAddress(0), 0) //count enable.
    , UIE(registerAddress(0x0C), 0) //update control
    , UIF(registerAddress(0x10), 0) //update indicator
    , OPM(registerAddress(0), 3)  //one pulse mode
    , UG(registerAddress(0x14),0) //force update
    , PSC(registerAddress(0x28))  //prescalar, divides typically max clock/2
    , ARR(registerAddress(0x2C))  //limit register
  {
    //#nada
  }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
  void init() const;
#pragma clang diagnostic pop
  /** input to timer's 16 bit counter in integer Hz,
    * appears to truncate rather than round.*/
  unsigned baseRate() const;


  /** sets the prescalar to generate the given hz.*/
  void setPrescaleFor(double hz) const;
  /** set cycle length in units determined by baseRate and prescale:*/
  void setCycler(unsigned ticks) const;
  unsigned getCycler() const;
  /** @returns rate at which timer will overflow if free running. */
  float overflowRate() const;
  unsigned ticksForMillis(unsigned ms) const;
  unsigned ticksForMicros(unsigned us) const;
  unsigned ticksForSeconds(double secs) const;
  unsigned ticksForHz(double Hz) const;
  float secondsInTicks(unsigned ticks) const;
  enum ExternalInputOption {
    Xor, CH1, CH2
  };

  /**
    * count the given external input, interrupt on overflow but keep counting
    */
  void configureCountExternalInput(enum ExternalInputOption, unsigned filter = 0) const;

  /** most uses will have to turn on some of the interrupts before calling this function.*/
  void beRunning(bool on = true) const {
    enable = on;
  }

  inline void clearEvents() const {
    word(0x10) = 0;
  }

  /**
    * access time for this guy was critical in first use.
    */
  inline u16 &counter() const {
    return Ref<u16>(registerAddress(0x24));
  }

 /** clear the counter and any flags then enable counting */
  void startRunning() const;

  inline void Interrupts(bool on) const {
    if (on) {
      irq.enable();
    } else {
      irq.disable();
      irq.clear();//and clear any pending (hopefully cures a stepper control issue of not freewheeling on de-configuration)
    }
  }

  void update() const;
  float getHz() const;
};

/** ccunit pattern for pwm with active at start of cycle */
constexpr uint8_t PwmEarly = 0b0'110'1'0'00;

struct CCUnit {
  const Timer &timer;
  const unsigned zluno;
  const ControlWord ticker;

  constexpr CCUnit(const Timer &_timer, unsigned _ccluno) : timer(_timer), zluno(_ccluno - 1), ticker(timer.registerAddress(0x34 + 4 * zluno)) {
    //nothing to do here
  }

  /** set the mode of this capture compare unit */
  void setmode(u8 cc) const;

  bool happened() const {
    return timer.bit(0x10, zluno + 1);
  }

  inline void clear() const {
    timer.bit(0x10, zluno + 1) = 0;
  }

  inline void IE(bool on) const {
    timer.bit(0x0C, zluno + 1) = on;
    //bit 0 must also be on but that is inappropriate to do here.
  }

  inline void Update() const {
    timer.bit(0x14, zluno + 1) = 1;
  }

public:
  /** unguarded tick setting, see saturateTicks() for when you can't prove your value will be legal.*/
  inline void setTicks(unsigned ticks) const {
    ticker = u16(ticks);
  }

  //some cc units have complementary outputs:
  bool amDual() const;
  /** set output polarity, and enable feature */
  void takePin(bool activehigh = true) const;

  //force on or off using cc config rather than gpio.
  void force(bool active) const;
};

/** uses ARR register to divide clock and give an interrupt at a periodic rate*/
class PeriodicInterrupter : public Timer {
public:
  PeriodicInterrupter(unsigned stLuno);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
  /**  sets the interrupts then chains */
  void beRunning(bool on = true) const;
#pragma clang diagnostic pop
  /** @overload */
  void restart(unsigned ticks) const;
  void restartHz(double hz) const; //todo:L pull up hierarchy
};

/*

  * timer used to indicate an interval.
  * rather than using a count down and interrupt on termination
  * we use a capture/compare unit so that we get a hardware indication
  * and can also measure the latency in dealing with it, as the counter keeps on counting.
  */
struct DelayTimer : public Timer {
  const CCUnit cc;

  constexpr DelayTimer(unsigned luno, unsigned ccluno) : Timer(luno), cc(*this, ccluno) {}

  void init(int hzResolution) const;
  const CCUnit &setDelay(int ticks) const;

  /** restart timeout*/
  inline void retrigger(void) const {
    startRunning(); //as long as this clears all possible interrupt sources then our retrigger can use it.
  }
};

/**  todo: untested use of OPM instead of interrupt */
class Monostable : public Timer {
public:
  constexpr Monostable(unsigned stLuno) : Timer(stLuno) {
    //#nada
  };
  void setPulseWidth(unsigned ticks) const;
  void setPulseMicros(unsigned microseconds) const;
public:
  void retrigger() const {
    OPM=1;
    startRunning();
    UIE = 1;
  }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
  //stuff that was formerly done in constructor
  void init() const;
#pragma clang diagnostic pop
};

/** Timer used to count pulses.*/
struct PulseCounter : public Timer {
  const CCUnit cc;

  /**extended 32 bit count, includes hardware component coordinated with overflow count.
    * only fully valid after stop() is called.
    *
    */
  unsigned count;

  PulseCounter(int timerLuno, Timer::ExternalInputOption channel, unsigned filter = 0) : Timer(timerLuno), cc(*this, channel == 2 ? 2 : 1) {
    configureCountExternalInput(channel, filter);
  }

  /** to be called from the timer's isr*/
  void isr() {
    clearEvents(); //kill all possible interrupts, in case we accidentally enable unexpected ones.
    count += 1 << 16; //or add in the reload value if we don't do a binary divide (ARR register not FFFF).
  }

  /**use this to continue after having been paused with stop, can't call it continue as that is a keyword :)*/
  void proceed() const {
    clearEvents();
    beRunning(true);
  }

  void start() {
    count = 0;
    startRunning();
  }

  void stop() {
    beRunning(false);
    count += counter();
  }

  /** enable interrupts for rollover.*/
  void configure(u8 priority) const {
//    cc.pin().DI('D'); //pulling down as these are usually low
    UIE = 1;
    cc.IE(1);
    irq.setPriority(priority);
  }
};

#pragma clang diagnostic pop
