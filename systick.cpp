#include "systick.h"

#include "peripheraltypes.h"

#include "nvic.h"
#include "clocks.h"
#include "minimath.h"  //safe division functions

#include "tableofpointers.h"  //*RefTable

MakeRefTable(SystemTicker);

namespace SystemTimer {
//when the following were simple static's Rowley would not show them.
  static SysTicks milliTime(0); //storage for global tick time.
  static unsigned macroTime(0); //extended range tick time
}
using namespace SystemTimer;

HandleFault(15) { //15: system tick
  ++milliTime;
  if (milliTime == 0) {
    //we have rolled over and anything waiting on a particular value will have failed
    ++macroTime;//but rollover of this is not going to happen for decades.
  }
  ForRefs(SystemTicker) {
    (**it)();
  }
}

struct SysTicker {
  volatile unsigned enableCounting: 1; //enable counting
  unsigned enableInterrupt: 1; //enable interrupt
  //note: some implementation do NOT implement this bit! Hopefully it read back as 0 even if you try to set it.
  unsigned fullspeed: 1; //1: main clock, 0: that divided by 8 (St's choice, ignore their naming)
  unsigned  : 16 - 3;
  volatile unsigned rolledOver: 1; //indicates rollover, clears on read
  unsigned  : 32 - 17;

  unsigned reload; //(only 24 bits are implemented) cycle = this+1.

  unsigned value; //
  //following is info chip provides to user, some manufacturers are off by a power of 10 here.
  unsigned tenms: 24; //value to get 100Hz
  unsigned  : 30 - 24;
  unsigned refIsApproximate: 1;
  unsigned noref: 1; //1= no ref clock

  bool start(u32 reloader) {
    enableInterrupt = 0;
    enableCounting = 0;
    reload = reloader - 1; //for more precise periodicity
    bool hack = rolledOver; //reading clears it
    enableCounting = 1;
    enableInterrupt = 1;

    return hack; //just to ensure optimizer doesn't eliminate read of rolledOver
  } /* start */

  SysTicks ticksPerSecond() const {
    unsigned effectiveDivider = reload + 1;

    if (!fullspeed) {
      effectiveDivider *= 8;
    }
    return rate(clockRate(CPU), effectiveDivider);
  }

  SysTicks ticksForMicros(unsigned us) const { //boost range for 180 seconds used by cg2 timeouts.
    return (uint64_t(us) *uint64_t( ticksPerSecond())) / 1000000;
  }

  SysTicks ticksForMillis(unsigned ms) const {
    return (ms * ticksPerSecond()) / 1000;
  }

  SysTicks ticksForHertz(float hz) const {
    return ratio(ticksPerSecond(), hz);
  }
};

soliton(SysTicker, 0xE000E010);

namespace SystemTimer {

  void disable() {
    theSysTicker.enableInterrupt = false;
  }


/** start ticking at the given rate.*/
  void startPeriodicTimer(unsigned persecond) {
    theSysTicker.fullspeed = 0;//todo:1 pass an option?
    //lpc has a programmable divider, not yet accommodated by this code.
    Hertz num =
#if DEVICE == 103 || DEVICE == 407
      theSysTicker.fullspeed ? clockRate(CPU) :  //STM addition
      #endif
      clockRate(AHB1) / 8;//standard defined by ARM
    theSysTicker.start(rate(num, persecond));
  }

  double secondsForTicks(SysTicks ticks) {
    return ratio(double(ticks), double(theSysTicker.ticksPerSecond()));
  }

  double secondsForLongTime(SysTime ticks) {
    return ratio(double(ticks), double(theSysTicker.ticksPerSecond()));
  }

  SysTicks ticksForSeconds(float sec) {
    if (sec <= 0) {
      return 0;
    }
    return theSysTicker.ticksForMicros(unsigned(sec * 1000000));
  }

  SysTicks ticksForMillis(int ms) {
    if (ms <= 0) {
      return 0;
    }
    return theSysTicker.ticksForMillis(ms);
  }

  SysTicks ticksForMicros(int us) {
    if (us <= 0) {
      return 0;
    }
    return theSysTicker.ticksForMicros(us);
  }

  SysTicks ticksForHertz(float hz) {
    return theSysTicker.ticksForHertz(hz);
  }

/** time since last rollover, must look at clock configuration to know what the unit is. */
  SysTicks snapTime() {
    return theSysTicker.reload - theSysTicker.value; //'tis a downcounter, want time since reload.
  }

  SysTicks snapTickTime() {
    //#some of the superficially gratuitous stuff in here exists to minimize the clocks spent with counting disabled.
    SysTicks snapms;
    SysTicks snaptick;
    theSysTicker.enableCounting = 0; //can't use bitlock on a field in a struct :(
    snapms = milliTime;
    snaptick = theSysTicker.value;
    theSysTicker.enableCounting = 1;
    //add some to systick to compensate for the dead time of this routine.
    snaptick -= 6;//counted clocks between the disable and enable operations
    //todo: might have skipped a tick, and failed to execute the isr which could cause some usages to have an unexpected delay or extension.
    return ((snapms + 1) * (theSysTicker.reload + 1)) - snaptick;
  }

  SysTime snapLongTime() {//this presumes little endian 64 bit integer.
    theSysTicker.enableCounting = 0; //can't use bitlock on a field in a struct :(
    SysTime retval = milliTime | (SysTime(macroTime) << 32);//need a hack to get compiler to be efficient here.
    theSysTicker.enableCounting = 1; //todo:3 add some to systick to compensate for the dead time of this routine.
    return retval;
  }

  unsigned tocks() {
    return milliTime;
  }

  void setPriority(unsigned int TickPriority) {
    setInterruptPriorityFor(-15, TickPriority);
  }
}

//end of file
