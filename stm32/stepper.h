#pragma once

#include "timer.h" //a dedicated hardware timer is used for each stepper for precise operation
#include "stm32.h"
#include "steppercontrol.h"
#include "pulseinput.h"
#include "twiddler.h"
#include "gpio.h"

//replace with functional once STLP quits being cranky
class Mechanism {
public:
  /** true step in positive direction else step in negative. */
  virtual void operator ()(bool positive)=0;
  /** true: power up, false: power down */
  virtual void operator =(bool enabled)=0;
  /** @returns state of poweredness */
  virtual operator bool() const=0;
};

class TwoPinMechanism : public Mechanism {
  const OutputPin &stepPos;
  const OutputPin &stepNeg;
  const OutputPin &powerPin;

public:
  TwoPinMechanism(const OutputPin &stepPos,const OutputPin &stepNeg,const OutputPin &powerPin);
  /** true step in positive direction else step in negative. */
   void operator ()(bool positive) {
     if(positive) stepPos=1; else stepNeg=1; //todo: we need a pulse width
   };
  /** true: power up, false: power down */
   void operator =(bool enabled){
     powerPin=enabled;
   }
  /** @returns state of poweredness */
   operator bool() const{
     return powerPin.actual();
   };
};

/** stepper motor driver
 * TODO: pipeline the step computation so that the ISR just loads a value and sets a flag to tell the main loop to compute another one.
 *  if the "need new value" is set when the ISR begins we do a simple restart wtihout reload and increment an error counter.
 * */

struct Motion {
  int desiredStep; //where we are going to stop.
  int actualStep; //updated when step is complete, not when issued.
  int direction; //+1,0,-1
  bool completed; //covers timing gaps where 'direction' is 0 but we expect to move on the next tick.
  void init(void){
    setLocation(0);
    direction = 0;
    completed = true;
  }

  bool active(void){
    return direction != 0 || !completed;
  }

  Motion(){
    init();
  }

  void setLocation(int arg){
    actualStep= desiredStep= arg;
  }
};

/** this controls the step rate dynamically while moving */
struct Ramper {
  int stepsRemaining;//temporarily negative at times.
  u32 stepticks; //timer reload value, for each step
  //can't afford to use floating point in code that runs from ISR, and don't want to gate off an interrupt so code that shares ISR variables also should not use floating point.
  //... so we convert float into float-via-averaging-integers
  u32 startticks;
  u32 endticks;
  u32 numSteps; //ramp tracker
  //subtract or add this amount per tick for accel/decel respectively
  PwmModulator dither; //delta ticks inside here.
  /** adjust step time for step about to be initiated, return that time*/
  inline u32 stepped(bool suppress=false)ISRISH;//#ISR
  inline void start(void);//#ISR
  /**@return whether we are at a speed where we can change direction or start or stop.*/
  inline bool crawling() ISRISH {//#ISR
    return stepticks>=startticks;//'>' deals with non-atomic change in velocity settings
  }
  /**convert doubles to integers for isr use.*/
  void apply(GasPedal&v, Timer&timer);//NOT called in the ISR
  bool apply(StepAccess &free, Timer&timer);
};

/** the homing process */
struct Homage {
  bool hasHomed;
  enum Stages {
    NotHoming=0,//implicitly used as "is homing"
    Center, Rewind, Forward2Fall, Backward2Start,
    HomingFailed
  } oming; //state machine index
  bool inProgress(bool orFaulted=true){
    return orFaulted? oming != NotHoming: (oming>NotHoming && oming<HomingFailed);
  }
};

class Stepper : public PeriodicInterrupter {
public:
  bool suppressedForDebug;
protected:
//  PositionerSettings &s;
  Ramper r;
  StepperControl c;
  Motion m;
  Homage h;//formerly part of StepperControl, needed pure separation of control and status.
  /** cached value of s.g.stepsPercycle */
  int circularity; //must be signed for proper math, although value will always be positive or 0.

  void startZeroing(int someIndex);
private:

public: //public for diagnostic access.
  PulseInput &mark;
  Mechanism &stepper;

public://public for isr linkage, do not call directly!
  void onDone(void) ISRISH ;//mingw compiler segfaults optimizing this method! Note: blank defines apparently define to '1'
private://routines exclusively called by isr
  bool nextStep() ISRISH;
  inline void pulse(/*int direction,u32 speed*/)ISRISH;
  void moveCompleted()ISRISH;

  int circularize(int &step) const ISRISH;
  void killProcedures();
protected:
  /** idiot check then cache as it is frequently referenced in ISR's*/
  void setCircularity(unsigned cyclelength);
  void setLocation(int locationNow);
  void hfailed(bool powered);
  void calFailed();
  void power(bool beon);

public:
//  void report(PositionerReport &r);
  void reportMore(MotorReport &report);
protected:  /** called in an ISR*/
  bool onMotionCompleted(bool normal /*else homed*/);
  void onMark(/*int direction,*/bool entered) ISRISH;//#implements Demon
  void onSizingComplete();

public:
  Stepper(unsigned timerLuno , Mechanism &&pins, PulseInput &&index);
  void init(void);
  void doLogic();
public:
  int location(void){
    return m.actualStep;
  }
  bool atLocation(u32 step);
  /** @return whether motor is or <i>is about to be</i> stepping*/
  inline bool inMotion(void)ISRISH{
    return m.active(); //direction should always be non-zero when braking is non-zero.
  }

  /** target and whether to rehome when you get there*/
  void absmoveto(int desired)ISRISH;
  void relmoveto(int desired)ISRISH;
};

