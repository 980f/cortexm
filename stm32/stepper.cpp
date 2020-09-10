#include "stepper.h"
#include "wtf.h"

#ifdef DebugStepper
#include "debugio.h"
#endif

#include "minimath.h"
//4debug
#if DebugStepper > 1
#include "circularindexer.h"
Motion lookback[100];
CircularIndexer <Motion> tracer(lookback, sizeof(lookback));
void trace(Motion&m){
  tracer.next() = m;
}
#else
#define trace(m)
#endif
//end debug.

void Stepper::power(bool beon) {
  stepper = beon;
}

void Stepper::pulse() {
  stepper(m.direction);
  restart(r.stepticks);
}

void Stepper::hfailed(bool powered) {
  h.oming = Homage::HomingFailed; //make sure we stay that way.
  h.hasHomed = false;
  power(powered);
}

void Stepper::nextStep() { //
//  int dirWas = m.direction; //remember for ramp logic
  r.stepsRemaining = m.desiredStep - m.actualStep;
  r.stepped();
  int dirNew = signabs(r.stepsRemaining);

//  bool optimal = (r.stepsRemaining <= circularity / 2);
//  if (!optimal) {
//    r.stepsRemaining = circularity - r.stepsRemaining;
//    if (r.stepsRemaining < 0) {
//      wtf(1234);
//    }
//    dirNew = -dirNew;
//  }

  if (m.direction != dirNew) {
    if (!r.crawling()) {//we need to decelerate, so don't change direction yet
      gentleHalt();
    } else {
      irq.disable();//we are slow enough that this shouldn't lose anything
      m.desiredStep = target;
      m.direction = dirNew;
      r.start();
      if (m.direction != 0 ) { //then actually begin motion
        power(1);//but we don't power down when we are done, that is not our responsibility
        irq.enable();
        pulse();//the first step in the new direction.
      }
    }
  } /* want to change direction */

} /* recomputeMotion */


/** ISR called at end of each step pulse.
 * It modifies the actualStep and records home sensor changes */
void Stepper::onDone() { // on step done
  //this is the isr, no locking needed.
  if (flagged(b->updateHappened)) {
    if (suppressedForDebug) {
      irq.disable();
      return;
    }
    if (c.a.run) {//stepping forever for debug
      pulse();
      return;
    }
    if (m.direction != 0) {//we took a step, else just watching index input
      if (mark.check(m.direction == 1, m.actualStep)) {//record the pin as precisely as we can
        mark.edgeCount++;
      }
      m.actualStep += m.direction;//update "where we are"
      m.completed = m.actualStep == m.desiredStep;
      if (!m.completed) {//not done yet
        pulse();
      }
    }//end "just finished a step"
    else { //stop interrupting!
      irq.disable();
    }
  } else { //unexpected timer interrupt
    //todo:2 kill the enables as well as the interrupt source.
  }
} /* onDone */

void Ramper::start() {
  stepticks = startticks;
}

/** called when a step is taken*/
u32 Ramper::stepped(bool suppress) {
  if (!suppress) {
    //no lock, only called from ISR
    if (unsigned(stepsRemaining) <= numSteps) { //then we need to be decelerating
      stepticks += dither.pwm();
      if (stepticks > startticks) {
        start();//actually this is the end but the functionality of 'start' is to set the system for slowest speed.
      }
    } else if (stepticks > endticks) { //need to accelerate
      stepticks -= dither.pwm();
      if (stepticks < endticks) {
        stepticks = endticks;
      }
    }
    //#else we leave stepticks alone
  }
  return stepticks;
} /* stepped */

void Ramper::apply(GasPedal &v, const Timer &timer) {
  startticks = timer.ticksForHz(v.start);
  endticks = timer.ticksForHz(v.cruise);
  int deltaTicks = startticks - endticks;
  numSteps = v.acceleration();//number of steps to move from one speed to the other
  dither.setRatio(ratio(double(deltaTicks), numSteps));
  start();//get a valid value, else the startup logic goes wonky.
}

bool Ramper::apply(StepAccess &a, const Timer &timer) {
  a.run = a.hz != 0;
  if (a.run) {
    a.clockwise = a.hz > 0;
    a.ticks = timer.ticksForHz(a.clockwise ? a.hz : -a.hz);
    endticks = startticks = a.ticks;
    dither.setRatio(0.0);
    start();//get a valid value, else the startup logic goes wonky.
    return true;
  } else {
    return false;
  }
}

Step Stepper::circularize(Step &step) const {
  return step = fast_modulus(step, circularity);
}

void Stepper::absmoveto(Step desired) {
  if(changed(target,circularize(desired))){
    //if moving then set immediate target to the step at which speed is slow enough to change direction
    if(m.active()&&!r.crawling()){
      gentleHalt();
    } else {
      m.desiredStep=target;
      beRunning(1);//# this is needed to unfreeze us after a calFailed.
    }
  }
}

void Stepper::gentleHalt()  {//set desired to minimum of where we will be crawling
  auto gentlehalt=min(this->r.stepsRemaining, this->r.numSteps);
  this->m.desiredStep= this->m.actualStep + this->m.direction * gentlehalt;
}

void Stepper::relmoveto(int desired) {
  absmoveto(m.actualStep + desired);
}


void Stepper::setLocation(Step arg) {
  m.setLocation(arg);
}

void Stepper::setCircularity(unsigned cyclelength) {
  if (cyclelength <= 2) {//2 allows us to use stepper as a relay drive.
    wtf(4100);
    return;
  }
  circularity = cyclelength; //#converts to signed.
  mark.cyclelength = circularity;//cache
}

void Stepper::home() {
  h.hasHomed=false;//the 'forget'
  setLocation(0);//makes following math easier
  if (mark.isActive()) {
    mark.edgeCount=-1;//this works as we increment before checking it and only check after a change.
    h.oming = Homage::Forward2Fall;
    m.direction=+1;
    r.stepsRemaining=c.h.widest;
  } else {
    mark.edgeCount=0;
    h.oming = Homage::Backward2Start;
    m.direction=-1;
    r.stepsRemaining=circularity+c.h.widest;
  }
  //todo: more needed here, need to arrange for circularity+hysteresis steps before giving up.
  nextStep();
}

/** any time a control might have changed ... or any interrupt which includes system timer tick */
void Stepper::doLogic() {
  if (suppressedForDebug) {
    return;
  }
  if (c.h.notConfigured()) {
    return;
  }
  if (!c.v.isValid()) {
    return;
  }

  if (h.inProgress(false)) {//if actively homing
    //todo: inspect mark.edgeCount in each state to see where we are
    switch(h.oming) {
    case Homage::Backward2Start:
      h.oming=Homage::Forward2Fall;
      relmoveto(+c.h.widest);
      return;
    case Homage::Forward2Fall:
      h.oming=Homage::Rewind;
      relmoveto(-c.h.widest);
      return;
    case Homage::Rewind:
      h.oming=Homage::Center;
      absmoveto(mark.center(1)+c.h.offset);//take center of present direction, skew is deemed due to windup, not sensor hysteresis
      return;
    case Homage::Center:
      if(m.completed){
        h.oming=Homage::NotHoming;
        absmoveto(target);
        return;
      }
      break;
    default: //on invalid codes
      wtf(9900+h.oming);
      h.oming=Homage::HomingFailed;
      break;//try to continue
    } /* switch */
  }

  if(!m.completed){//isr has launched another step
    nextStep();
    return;//this return keeps us from processing configuration changes while in motion.
  }

   //tired of fighting with this. If commands are received while moving the issuer of commands screwed up. IRQLOCK(irq); //changes things the isr references.
  if (c.v.wasModified()) {
    r.apply(c.v, *this);//convert Hz info ticks per step.
  }

  if (c.wasModified()) {//diagnostic access
    if (r.apply(c.a, *this)) {//if starting free-running
      m.direction = c.a.clockwise ? 1 : -1;
      //arrange for things to be quiet when we stop:
      h.hasHomed = false;
      return;
    }
  }

  if (c.a.wasModified()) {//diagnostic access
    absmoveto(c.a.position);
  }

  if (c.h.wasModified()) {//for diags/setup, normal use won't trigger this.
    //do nothing
  }
} /* doLogic */

bool Stepper::atLocation(Step step) const {
  return !inMotion() && h.hasHomed && u32(m.actualStep) == step;
}

void Stepper::init() {
  setPrescaleFor(50000); //divisor needed to get the slowness we may need while still having nice decimal roundoff on values.
  m.init();
  //leave isr dead until we have some live configuration.
}

void Stepper::reportMore(MotorReport &mr) {
  mr.motionCode = h.inProgress(false) ? 2 : (inMotion() ? 1 : 0);
  mr.homingStage = h.oming;
  mr.hasHomed = h.hasHomed;
  mr.location = m.actualStep;
  mr.target = m.desiredStep;
  mark.markReport(mr.markReport);
}

//////////////////////
TwoPinMechanism::TwoPinMechanism(const OutputPin &stepPos, const OutputPin &stepNeg, const OutputPin &powerPin) : stepPos(stepPos), stepNeg(stepNeg), powerPin(powerPin) {}

Stepper::Stepper(unsigned timerLuno, Mechanism &&pins, PulseInput &&index) : PeriodicInterrupter(timerLuno), stepper(pins), mark(index) {}

//end of file
