#include "systick.h"

#include "peripheraltypes.h"

#include "nvic.h"
#include "clocks.h"
#include "minimath.h"  //safe division functions

#include "tableofpointers.h"

MakeRefTable(SystemTicker);

namespace SystemTimer {
//when the following were simple static's Rowley would not show them.
static u32 milliTime(0); //storage for global tick time.
static u32 macroTime(0); //extended range tick time
}
using namespace SystemTimer;

HandleFault(15){ //15: system tick
  ++milliTime;
  if(milliTime == 0) {
    //we have rolled over and anything waiting on a particular value will have failed
    ++macroTime;//but rollover of this is not going to happen for decades.
  }
  ForRefs(SystemTicker){
    (**it)();
  }
}


struct SysTicker {
  volatile unsigned int enableCounting : 1; //enable counting
  unsigned int enableInterrupt : 1; //enable interrupt
  //note: some implementation do NOT implement this bit! Hopefully it read back as 0 even if you try to set it.
  unsigned int fullspeed : 1; //1: main clock, 0: that divided by 8 (St's choice, ignore their naming)
  unsigned int : 16 - 3;
  volatile unsigned int rolledOver : 1; //indicates rollover, clears on read
  unsigned int : 32 - 17;

  u32 reload; //(only 24 bits are implemented) cycle = this+1.

  u32 value; //
  //following is info chip provides to user, some manufacturers are off by a power of 10 here.
  unsigned int tenms : 24; //value to get 100Hz
  unsigned int : 30 - 24;
  unsigned int refIsApproximate : 1;
  unsigned int noref : 1; //1= no ref clock

  bool start(u32 reloader){
    enableInterrupt = 0;
    enableCounting = 0;
    fullspeed = 1;

    reload = reloader - 1; //for more precise periodicity
    bool hack = rolledOver; //reading clears it
    enableCounting = 1;
    enableInterrupt = 1;

    return hack; //just to ensure optimizer doesn't eliminate read of rolledOver
  } /* start */

  u32 ticksPerSecond(void){
    u32 effectiveDivider = reload + 1;

    if(!fullspeed) {
      effectiveDivider *= 8;
    }
    return rate(clockRate(-1), effectiveDivider);
  }

  u32 ticksForMicros(u32 us){
    return (us * ticksPerSecond()) / 1000000;
  }

  u32 ticksForMillis(u32 ms){
    return (ms * ticksPerSecond()) / 1000;
  }

  u32 ticksForHertz(float hz){
    return ratio(ticksPerSecond(), hz);
  }

};

soliton(SysTicker, 0xE000E010);

namespace SystemTimer {

/** start ticking at the given rate.*/
void startPeriodicTimer(u32 persecond){
  //todo:2 fullspeed is hardcoded to 1 downstream of here, need to take care of that.
  theSysTicker.fullspeed = 1;
  //lpc has a programmable divider 
  if(!theSysTicker.fullspeed) {//!! stm32 specific (although others copy it)
    persecond *= 8; // times 8 here instead of /8 in the rate computation.
  }
  u32 num=clockRate(-1);
  theSysTicker.start(rate(num, persecond));
}

double secondsForTicks(u32 ticks){
  return ratio(double(ticks), double(theSysTicker.ticksPerSecond()));
}

double secondsForLongTime(u64 ticks){
  return ratio(double(ticks), double(theSysTicker.ticksPerSecond()));
}

u32 ticksForSeconds(float sec){
  if(sec<=0){
    return 0;
  }
  return theSysTicker.ticksForMillis(u32(sec * 1000));
}

u32 ticksForMillis(int ms){
  if(ms<=0){
    return 0;
  }
  return theSysTicker.ticksForMillis(ms);
}

u32 ticksForMicros(int ms){
  if(ms<=0){
    return 0;
  }
  return theSysTicker.ticksForMicros(ms);
}

u32 ticksForHertz(float hz){
  return theSysTicker.ticksForHertz(hz);
}

/** time since last rollover, must look at clock configuration to know what the unit is. */
u32 snapTime(void){
  return theSysTicker.reload - theSysTicker.value; //'tis a downcounter, want time since reload.
}

u32 snapTickTime(void){
  //#some of the superficially gratuitous stuff in here exists to minimize the clocks spent with counting disabled.
  u32 snapms;
  u32 snaptick;
  theSysTicker.enableCounting = 0; //can't use bitlock on a field in a struct :(
  snapms=milliTime;
  snaptick=theSysTicker.value;
  theSysTicker.enableCounting = 1; 
  //add some to systick to compensate for the dead time of this routine.
  snaptick-=6;//counted clocks between the disable and enable operations
  //todo: might have skipped a tick, and failed to execute the isr which could cause some usages to have an unexpected delay or extension.
  return ((snapms + 1) * (theSysTicker.reload + 1)) - snaptick;
}

u64 snapLongTime(void){//this presumes  little endian 64 bit integer.
  theSysTicker.enableCounting = 0; //can't use bitlock on a field in a struct :(
  u64 retval=milliTime |(u64(macroTime)<<32);//need a hack to get compiler to be efficient here.
  theSysTicker.enableCounting = 1; //todo:3 add some to systick to compensate for the dead time of this routine.
  return retval;
}
}

//end of file
