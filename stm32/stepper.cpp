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

void Stepper::power(bool beon){
  stepper=beon;
}

void Stepper::pulse(/*int direction, u32 speed*/){
  stepper(m.direction);
  restart(r.stepped());
  //todo: set flag for pipelined "compute next step"
}

void Stepper::hfailed(bool powered){
  h.oming = Homage::HomingFailed; //make sure we stay that way.
  h.hasHomed=false;
  power(powered);
}

bool Stepper::nextStep(){ //a fragment of the ISR
  int dirWas = m.direction; //remember for ramp logic
  int stepsWere=r.stepsRemaining;
  if(stepsWere<0){
    wtf(12345);
  }
  r.stepsRemaining = m.desiredStep - m.actualStep;
  int dirNew=signabs(r.stepsRemaining);

  bool optimal= (r.stepsRemaining <= circularity / 2);
  if(!optimal){
    r.stepsRemaining = circularity-r.stepsRemaining;
    if(r.stepsRemaining<0){
      wtf(1234);
    }
    dirNew=-dirNew;
  }

  if(dirWas!=dirNew) {
    if(!r.crawling()){//we need to decelerate, so don't change direction yet
      r.stepsRemaining = min(stepsWere,r.numSteps); //to ensure ramp down before change of direction
    } else {
      m.direction = dirNew;
      if(m.direction != 0 && !dirWas) { //then actually begin motion
        power(1);
        r.start();
      }
    }
  } /* want to change direction */

  if(m.direction!=0){
    m.actualStep+=m.direction;
    circularize(m.actualStep);
    pulse();
    return true;
  } else {
    return false;
  }
} /* recomputeMotion */


bool Stepper::onMotionCompleted(bool normal /*else homed*/){//#ISR stepper motor's
  //a hook to trigger a followup motion.
}

void Stepper::moveCompleted(){
  if(h.inProgress()) {
    if(h.oming==Homage::Center){//last stage of homing
      h.oming=Homage::NotHoming;
      h.hasHomed = true;
      m.completed = onMotionCompleted(0);
    } else {
      hfailed(0);
    }
  } else {//stopped while not homing
    m.completed = onMotionCompleted(1);
  }
  power(!m.completed);
}

/** ISR called at end of each step pulse */
void Stepper::onDone(){ // on step done
  //this is the isr, no locking needed.
  if(flagged(b->updateHappened)) {
    if(suppressedForDebug){
      irq.disable();
      return; //and a reset will be needed for this hardware to work, or some annoying jiggering with the debugger.
    }
    if(c.a.run){
      pulse();
      return;
    }
    if(m.direction != 0) {//we took a step, else just watching index input
      if(mark.check(m.direction == 1, m.actualStep)) {
        bool inslot=mark.isActive();
//        onMark(/*m.direction,*/inslot);
        if(!inslot){
          if(h.inProgress()){
            switch(h.oming) {
            case Homage::Backward2Start:
              h.oming=Homage::Forward2Fall;
              relmoveto(+c.h.widest);
              break;
            case Homage::Forward2Fall:
              h.oming=Homage::Rewind;
              relmoveto(-c.h.widest);
              break;
            case Homage::Rewind:
              h.oming=Homage::Center;
              absmoveto(mark.center(1)+c.h.offset);//take center of present direction, skew is deemed due to windup, not sensor hysteresis
              break;
            default: //on invalid codes
              wtf(9090);
              break;
            } /* switch */
          }
        }//end homing state transition
      }// end saw mark edge while moving
      if(!nextStep()){//then just finished the last step of a motion.
        moveCompleted();
      }
    }//end "just finished a step"
    else {//haven't been moving, might need to start.
      if(mark.changed()) { //#read without recording
        //loss of absolute location, but can't tell which way!
        //todo:1 this is misfiring on filter wheel, find out why later       h.hasHomed=false;
      }
      if(!nextStep()){//if we did NOT just start
        if(!m.completed){
          //then a zero setp move was requested
          moveCompleted();
        }
      }
    }
  } else { //unexpected timer interrupt
    //todo:2 kill the enables as well as the interrupt source.
  }
} /* onDone */


void Ramper::start(){ //called from ISR
  stepticks = startticks;
}

/** called when a step is taken*/
u32 Ramper::stepped(bool suppress){
  if(!suppress){
    //no lock, only called from ISR
    if(unsigned(stepsRemaining)<=numSteps) { //then we need to be decelerating
      stepticks += dither.pwm();
      if(stepticks>startticks){
        start();//actually this is the end but the functionality of 'start' is to set the system for slowest speed.
      }
    } else if(stepticks > endticks) { //need to accelerate
      stepticks -= dither.pwm();
      if(stepticks<endticks){
        stepticks=endticks;
      }
    }
    //#else we leave stepticks alone
  }
  return stepticks;
} /* stepped */

void Ramper::apply(GasPedal&v, Timer&timer){
  startticks = timer.ticksForHz(v.start);
  endticks = timer.ticksForHz(v.cruise);
  int deltaTicks=startticks-endticks;
  numSteps=v.acceleration();//number of steps to move from one speed to the other
  dither.setRatio(ratio(double(deltaTicks),numSteps));
  start();//get a valid value, else the startup logic goes wonky.
}

bool Ramper::apply(StepAccess &a, Timer&timer){
  a.run=a.hz!=0;
  if(a.run){
    a.clockwise=a.hz>0;
    a.ticks = timer.ticksForHz(a.clockwise?a.hz:-a.hz);
    endticks = startticks = a.ticks ;
    dither.setRatio(0.0);
    start();//get a valid value, else the startup logic goes wonky.
    return true;
  } else {
    return false;
  }
}

int Stepper::circularize(int &step) const{
  return step=modulus(step,circularity);
}

void Stepper::absmoveto(int desired){//#ISR as well as not.
  if(desired<0){
    desired+=circularity;//but don't do full modulus as that interferes with the initial full scan.
  }
  if(desired>circularity){
    desired-=circularity;
  }
  IRQLOCK(irq); //changes things the isr references.
  if(changed(m.desiredStep, desired)) {
    m.completed = false;//covers timing gap from command until next doLogic() invocation.
    trace(m);
  }
  beRunning(1);//# this is needed to unfreeze us after a calFailed.
}

void Stepper::relmoveto(int desired){
  absmoveto(m.actualStep+desired);
}

void Stepper::killProcedures(){
  h.oming=Homage::NotHoming;
}

/** redefine where we are, but don't move*/
void Stepper::setLocation(int arg){
  m.setLocation(arg);
}

void Stepper::setCircularity(unsigned cyclelength){
  if(cyclelength<=2){//2 allows us to use stepper as a relay drive.
    wtf(4100);
    return;
  }
  circularity = cyclelength; //#converts to signed.
  mark.cyclelength=circularity;//cache
}

/**any time a control might have changed ... or any interrupt which includes system timer tick*/
void Stepper::doLogic( ){
  if(suppressedForDebug){
    return;
  }
  if(c.h.notConfigured()){
    return;
  }
  if(!c.v.isValid()){
    return;
  }

  if(h.inProgress(false)){//if actively homing
    return; //command processing trashes centering
  }
  IRQLOCK(irq); //changes things the isr references.

  if(c.v.wasModified()){
    r.apply(c.v, *this);//convert Hz info ticks per step.
  }

  if(c.wasModified()){//diagnostic access
    if(r.apply(c.a,*this)){//if starting free-running
      m.direction=c.a.clockwise?1:-1;
      //arrange for things to be quiet when we stop:
      h.hasHomed=false;
      return;
    }
  }

  if(c.a.wasModified()) {//diagnostic access
    absmoveto(c.a.position);
  }

  if(c.h.wasModified()) {//for diags/setup, normal use won't trigger this.
    //do nothing
  }

} /* doLogic */

bool Stepper::atLocation(u32 step){
  return !inMotion()&&h.hasHomed&& u32(m.actualStep)==step;
}



void Stepper::init(){
  setPrescaleFor(50000); //divisor needed to get the slowness we may need while still having nice decimal roundoff on values.
  m.init();
  //leave isr dead until we have some live configuration.
}


void Stepper::reportMore(MotorReport &mr){
  IRQLOCK(irq); //to ensure coherence
  mr.motionCode= h.inProgress(false)?2:(inMotion()?1:0);
  mr.homingStage=h.oming;
  mr.hasHomed=h.hasHomed;
  mr.location= m.actualStep;
  mr.target= m.desiredStep;
  mark.markReport(mr.markReport);
}

//////////////////////
TwoPinMechanism::TwoPinMechanism(const OutputPin &stepPos,const OutputPin &stepNeg,const OutputPin &powerPin):stepPos(stepPos),stepNeg(stepNeg),powerPin(powerPin){}

Stepper::Stepper(unsigned timerLuno , Mechanism &&pins, PulseInput &&index):PeriodicInterrupter(timerLuno),stepper(pins),mark(index){}

//end of file
