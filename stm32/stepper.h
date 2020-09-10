#pragma once

#include "timer.h" //a dedicated hardware timer is used for each stepper for precise operation
#include "stm32.h"
#include "steppercontrol.h" //commands and config
#include "pulseinput.h" //home sensor type is misnamed
#include "twiddler.h"  //smooth transitions
#include "gpio.h"  //used by physical interface.

//might replace with functional once STLP quits being cranky
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
  //todo: own sub timer for pulse width.

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
 * biggest change from the past: pipeline the step computation so that the ISR just loads a value and sets a flag to tell the main loop to compute another one.
 *  if the "need new value" is set when the ISR begins we do a simple restart wtihout reload and increment an error counter.
 * */

/** defining type used for interface for all positions. Can be negative near home. */
using Step = int;
/** defining type used for speed logic */
using StepTick = unsigned ;

struct Motion {
  Step desiredStep; //where we are going to stop.
  /** incremented by isr when it has triggered a step.
  * polled by the main loop, compared to desiredstep target value and if different then a new step width is computed.
  * it can be the 'actual step' value, no reason for it not to be other than the hassle of checking direction.
  * doint it as counter+=direction makes that hassle trivial.
  */
  Step actualStep; //updated when step is complete, not when issued.
  int direction; //+1,0,-1
  bool completed; //covers timing gaps where 'direction' is 0 but we expect to move on the next tick.
  void init(void){
    setLocation(0);
    direction = 0;
    completed = true;
  }

  bool active(void) const{
    return direction != 0 || !completed;
  }

  Motion(){
    init();
  }

  /** declare the present physical location to be the given value */
  void setLocation(Step arg){
    actualStep= desiredStep= arg;
  }
};

/** this controls the step rate dynamically while moving */
struct Ramper {
  int stepsRemaining;//temporarily negative at times.
  /** present speed, loaded into timer for next step. */
  StepTick stepticks; //timer reload value, for each step
  //can't afford to use floating point in code that runs from ISR, and don't want to gate off an interrupt so code that shares ISR variables also should not use floating point.
  //... so we convert float into float-via-averaging-integers
  /** unstick/initial/crawling speed */
  StepTick startticks;
  /** maximum/cruise speed */
  StepTick endticks;
  /** from acceleration we compute number of ticks to get from startticks to endticks */
  unsigned numSteps; //ramp tracker
  /** subtract or add this amount per tick for accel/decel respectively */
  PwmModulator dither; //delta ticks inside here.
  /** adjust step time for step about to be initiated, return that time*/
  inline StepTick stepped(bool suppress=false);
  inline void start(void);
  /**@return whether we are at a speed where we can change direction or start or stop.*/
  inline bool crawling(){
    return stepticks>=startticks;//'>' deals with non-atomic change in velocity settings
  }
  /** apply configuration values, convert doubles to integers for isr use.*/
  void apply(GasPedal&v, const Timer&timer);//NOT called in the ISR
  /** apply diagonstic access */
  bool apply(StepAccess &free, const Timer&timer);
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
protected://to be extended by an indexed positioner.
  Step target;
  Ramper r;
  StepperControl c;
  Motion m;
  Homage h;//formerly part of StepperControl, needed pure separation of control and status.
  /** for circular devices this is the steps per revolution. Set to 0 for non-circular mechanisms */
  Step circularity; //must be signed for proper math, although value will always be positive or 0.

public: //public for diagnostic access.
  PulseInput &mark;
  Mechanism &stepper;

public://public for isr linkage, do not call directly!
  void onDone(void) ISRISH ;//mingw compiler segfaults optimizing this method! Note: blank defines apparently define to '1'
  /** fires off a step and sets the timer for the next interrupt using Ramper current value */
  inline void pulse() ISRISH;
private:
  /** carefully slowdown from where we are, decelerating so that we don't lose track of steps */
  void gentleHalt();
  /** see if we are done, if not adjust speed */
  void nextStep() ;
  /** keep parameters within bounds, but allow moves past home */
  Step circularize(Step &step) const;

protected:
  /** idiot check then cache as it is frequently referenced in ISR's*/
  void setCircularity(unsigned cyclelength);
  /** redefine where we are, but don't move*/
  void setLocation(Step locationNow);
  /** called when homing gives up */
  void hfailed(bool powered);
  /** interface power. Normally you turn it on and leave it on. We turn it off when homing fails so that a person can wiggle the device to see why the home sensor isn't working. */
  void power(bool beon);

public:
  Stepper(unsigned timerLuno , Mechanism &&pins, PulseInput &&index);
  void init();
  /** must be called when the step might have completed, and when any configuration might have changed.
   * so we do it in the main loop. */
  void doLogic();

  Step location(){
    return m.actualStep;
  }
  bool atLocation(Step step) const;
  /** @return whether motor is or <i>is about to be</i> stepping*/
  inline bool inMotion() const{
    return m.active(); //direction should always be non-zero when braking is non-zero.
  }
  void reportMore(MotorReport &report);

  /** drop present home status and start finding it all over again */
  void home();
  /** target and whether to rehome when you get there*/
  void absmoveto(Step desired);
  void relmoveto(Step desired);
};

