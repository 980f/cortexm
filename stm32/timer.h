#ifndef timerH
#define timerH

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
  */

#include "stm32.h"
#include "nvic.h"
#include "minimath.h"

#include "gpio.h" //for control of associated pins

/* between stm embedding a 16 bit only device in a 32 bit address space and not making it byte writable and
  * gcc insisting on using byte operations for accessing bit fields whenever possible
  * we can't simply make a struct that represents the hardware register content.
  */

typedef u16 *TIMER_DCB;

struct CC_BAND {
  unsigned int mode[2]; //determines meaning of fields that follow
  union {
    struct {
      unsigned int prescale[2];
      unsigned int filter[4]; //a code, mixture of divisor and n-in-a-row logic.
    }
    In;
    struct {
      unsigned int hairTrigger;
      unsigned int bufferPreload;
      unsigned int compareMode[3];
      unsigned int hardwareClearEnable;
    }
    Out;
  };
};

struct CCER {
  unsigned int Enabled;
  unsigned int Inverted;
  unsigned int NE; //+adv 1..3
  unsigned int NP; //+adv 1..3
};

struct TIMER_BAND {
  unsigned int enabled;
  unsigned int updateDisable;
  unsigned int URS;
  unsigned int oneshot;
  unsigned int countDown;
  unsigned int CMS[2];
  unsigned int bufferPreload;
  unsigned int CKD[2];
  unsigned int cr1unused[32 - 10];
  //TIMx_CR2
  unsigned int CCPC : 1; //+adv
  unsigned int cr2unused1;
  unsigned int CCUS; //+adv
  unsigned int CCDS;
  unsigned int MMS[3];
  unsigned int in1Xored;
  //output idle state:
  unsigned int OIS1; //+adv
  unsigned int OIS1N; //+adv
  unsigned int OIS2; //+adv
  unsigned int OIS2N; //+adv
  unsigned int OIS3; //+adv
  unsigned int OIS3N; //+adv
  unsigned int OIS4; //+adv
  unsigned int cr2unused2[32 - 15];
  //TIMx_SMCR
  unsigned int mode[3];
  unsigned int smcrunused1;
  unsigned int triggerSource[3];
  unsigned int syncTrigger;
  unsigned int triggerFilter[4]; //
  unsigned int triggerPrescaler[2]; // external trigger is prescaled by 1 << this
  unsigned int ECE;
  unsigned int ETRactivelow;
  unsigned int smcrunused[32 - 16];
  //TIMx_DIER
  unsigned int updateIE;
  unsigned int ccIE[4]; //cc#-1
  unsigned int comIE;

  unsigned int triggerIE;
  unsigned int brakeIE;
  unsigned int dmaOnUpdate;
  unsigned int dmaOncc[4]; //cc#-1
  unsigned int dmaOnCom;
  unsigned int dmaOnTrigger;
  unsigned int dieunused[32 - 15];
  //SR
  unsigned int updateHappened;
  unsigned int ccHappened[4];
  unsigned int comHappened;
  unsigned int triggerHappened;
  unsigned int brakeHappened;
  unsigned int srunused2;
  unsigned int ccOverrun[4];
  unsigned int srunused[32 - 13];
  //TIMx_EGR
  unsigned int fake_update; //UG
  unsigned int fake_cc[4];
  unsigned int fake_com;
  unsigned int fake_trigger;
  unsigned int fake_brake;
  unsigned int egrunused[32 - 8];

  CC_BAND cc0;
  CC_BAND cc1;
  unsigned int ccunused[16];

  CC_BAND cc2;
  CC_BAND cc3;
  unsigned int ccunused2[16];


  //TIMx_CCER
  CCER ccer[4];
  unsigned int ccerunused[32 - 16];
  unsigned int skipabunch[8 * 32 + 8 + 2];
  unsigned int OSSI; //+adv
  unsigned int OSSR; //+adv
  unsigned int brakeEnable; //+adv
  unsigned int brakePolarity; //+adv
  unsigned int AOE; //+adv
  unsigned int MOE; //+adv
  //rest aren't worth encoding, all are wide fields.

};

#include "debugger.h"

struct Timer {
  TIMER_BAND *b;
  TIMER_DCB dcb; //access as dcb[databook's offset/2] (beware hex vs decimal)
  const APBdevice apb;
  const Irq irq;
  unsigned luno; //handy for debug, and pin setter routine

  bool isAdvanced(void) const {
    return luno == 1 || luno == 8;
  }

  Timer(unsigned stLuno);
  /** input to timer's 16 bit counter in integer Hz,
    * appears to truncate rather than round.*/
  u32 baseRate(void) const;

  /** sets the prescalar to generate the given hz.*/
  void setPrescaleFor(double hz) const;
  /** set cycle length in units determined by baseRate and prescale:*/
  void setCycler(u32 ticks) const;
  u32 getCycler(void) const;
  double getHz(void) const;
  u32 ticksForMillis(u32 ms) const;
  u32 ticksForHz(double Hz) const;
  double secondsInTicks(u32 ticks)const;
  enum ExternalInputOption { Xor, CH1, CH2 };

  /**
    * count the given external input, interrupt on overflow but keep counting
    */
  void configureCountExternalInput(enum ExternalInputOption, unsigned filter = 0) const;

  /** most uses will have to turn on some of the interrupts before calling this function.*/
  void beRunning(bool on = true) const {
    b->enabled = on;
  }

  void clearEvents(void) const {
    dcb[8] = 0;
  }

  /**
    * access time for this guy was critical in first use.
    */
  inline u16 *counter(void) const {
    return &dcb[18];
  }

  /** clear the counter and any flags then enable counting */
  void startRunning(void) const {
    b->enabled = 0; //to ensure events are honored on very short counts when interrupted between clearing counts and clearing events.
    *counter() = 0;
    clearEvents();
    beRunning(true);
  }

  void UIE(bool on) const {
    b->updateIE = on;
  }

  void Interrupts(bool on){
    if(on) {
      irq.enable();
    } else {
      irq.disable();
    }
  }
};

struct CCUnit {
  const Timer&timer;
  unsigned zluno;
  CCUnit(const Timer&_timer, unsigned _ccluno);

  /** set the mode of this capture compare unit */
  void setmode(u8 cc) const;

  inline bool happened(void) const {
    return timer.b->ccHappened[zluno];
  }

  inline void clear(void) const {
    timer.b->ccHappened[zluno] = 0;
  }

  inline void IE(bool on) const {
    timer.b->ccIE[zluno] = on;
  }

  inline CCER& ccer(void) const {
    return timer.b->ccer[zluno];
  }

  int saturateTicks(int ticks) const { //todo:M replace with inline use of minimath::saturate
    if(ticks < 0) {
      return 0;
    } else if(ticks > 65535) {
      return 65535;
    }
    return ticks;
  }
private:
  inline u16 *ticker(void) const {
    return &timer.dcb[2 * zluno + 26];
  }
public:
  /** unguarded tick setting, see saturateTicks() for when you can't prove your value will be legal.*/
  inline void setTicks(unsigned ticks) const {
    *ticker() = u16(ticks);
  }

  inline u16 getTicks(void) const {
    return *ticker();
  }

  /** @return physical pin configuration object for this unit*/
  Pin pin(unsigned alt = 0, bool complementaryOutput = false) const;
  //some cc units have complementary outputs:
  bool amDual(void) const;
  //set polarity
  //force on or off using cc config rather than gpio.
  //if timer is #1 or #8 then there are more bits:
  void takePin(unsigned alt=0,bool inverted=0) const; //todo:M remove default
};

/** uses ARR register to divide clock and give an interrupt at a periodic rate*/
class PeriodicInterrupter : public Timer {
public:
  PeriodicInterrupter(unsigned stLuno) : Timer(stLuno){}
  /** @overload sets the interrupts then chains */
  void beRunning(bool on = true); //can't const as interrupts are manipulated
  /** @overload */
  void restart(u32 ticks); //can't const as interrupts are manipulated
  void restart(double hz); //can't const as interrupts are manipulated //todo:L pull up hierarchy
};

/*
  *
  *timer used to indicate an interval.
  * rather than using a count down and interrupt on termination
  * we use a capture/compare unit so that we get a hardware indication
  * and can also measure the latency in dealing with it, as the counter keeps on counting.
  */
struct DelayTimer : public Timer {
  const CCUnit cc;
  DelayTimer(int luno, int ccluno);

  void init(int hzResolution) const;
  const CCUnit& setDelay(int ticks) const;
  /** restart timeout*/
  inline void retrigger(void) const {
    startRunning(); //as long as this clears all possible interrupt sources then our retrigger can use it.
  }

};


/** Timer used to count pulses.*/
struct PulseCounter : public Timer {
  const CCUnit cc;

  /**extended 32 bit count, includes hardware component coordinated with overflow count.
    * only fully valid after stop() is called.
    *
    */
  int count;

  PulseCounter(int timerLuno, Timer::ExternalInputOption channel, unsigned filter = 0): Timer(timerLuno), cc(*this, channel == 2 ? 2 : 1){
    configureCountExternalInput(channel, filter);
  }


  /** to be called from the timer's isr*/
  void isr(void){
    clearEvents(); //kill all possible interrupts, in case we accidentally enable unexpected ones.
    count += 1 << 16; //or add in the reload value if we don't do a binary divide (ARR register not FFFF).
  }

  /**use this to continue after having been paused with stop, can't call it continue as that is a keyword :)*/
  void proceed(void) const {
    clearEvents();
    beRunning(1);
  }

  void start(void){
    count = 0;
    startRunning();
  }

  void stop(void){
    beRunning(0);
    count += *counter();
  }
  /**configure pin as pulled-down input and enable interrupts for rollover.*/
  void configure(u8 priority) const {
    cc.pin().DI('D'); //pulling down as these are usually low
    UIE(1);
    cc.IE(1);
    irq.setPriority(priority);
  }

};

#endif /* ifndef timerH */
