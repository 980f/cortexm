#include "positioner.h"

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

void Positioner::power(bool beon){
  if(powerPin.changed(!beon) && beon) { //#hard coded as low active pin
    //if the below is more than a mike or two then we need to do this wait via state machine.
    nanoSpin(400); //worst case of worst device's power up, belongs in art.h
  }
}

static const unsigned phaseTable[] = { 0b1001, 0b0101, 0b0110, 0b1010 }; //see schematic

void Positioner::pulse(/*int direction, u32 speed*/){
  phasor+=m.direction;
  output = phaseTable[phasor& 3];//actually touch the motor
  restart(r.stepped());
}

void Positioner::hfailed(bool powered){
  h.oming = Homage::HomingFailed; //make sure we stay that way.
  h.hasHomed=false;
  power(powered);
}

bool Positioner::nextStep(){ //a fragment of the ISR
  int dirWas = m.direction; //remember for ramp logic
  int stepsWere=r.stepsRemaining;
  if(stepsWere<0){
    wtf(12345);
  }
  r.stepsRemaining = m.desiredStep - m.actualStep;
  int dirNew=signabs(r.stepsRemaining);

  bool optimal=sizing|| (r.stepsRemaining <= circularity / 2);
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
    if(!sizing){//need to do more than a half rev
      circularize(m.actualStep);
    }
    pulse();
    return true;
  } else {
    return false;
  }
} /* recomputeMotion */


void Positioner::bumpIndex(int direction){
  lastKnownIndex+=direction;
  if(!calibrating()){ //need to verify number of indexes
    if(lastKnownIndex>=int(s.g.numIndexes)){
      lastKnownIndex=1;
    }
    if(lastKnownIndex<=0){
      lastKnownIndex=s.g.numIndexes;
    }
  }
}

void Positioner::onMark(bool entered){
  if(m.direction){//then moving
    if(entered){
      if(!h.oming){
        //todo:1 somehow detect glitch, not sure how to figure out distance since last mark, loc[] isn't validated yet as always being useful.
        bumpIndex(m.direction);
      }
    } else {//record width and center on exit of mark
      int width=mark.width(m.direction>0);
      //todo:1 somehow detect glitch, not sure how to figure out distance since last mark
      loc[lastKnownIndex].center=mark.center(m.direction>0);
      loc[lastKnownIndex].markWidth=width;
    }
  }
}

void Positioner::calFailed(){
  wtf(888);
  power(false);//don't want to burn up motor on failure
  beRunning(0);//no tray installed
}

void Positioner::startZeroing(int someIndex){
  zeroing=true;
  absmoveto(loc[someIndex].center);//don't worry about offset here.
}

void Positioner::onSizingComplete(){
  m.desiredStep=circularize(m.actualStep);
  if(lastKnownIndex<1){//if no mark found
    calFailed();
  }
  if(s.simpleHome()){
    startZeroing(lastKnownIndex);
  } else {
    int width=s.g.widthPer;//filter out single sample glitches
    int index=-1;
    for(int scan=lastKnownIndex;scan>0;--scan){//don't trust first (in time order) element
      if(width<loc[scan].markWidth){//prefer the higher index if two match
        width=loc[scan].markWidth;
        index=scan;
      }
    }
    if(index>0){
      startZeroing(index);
    } else {//no reasonably sized mark found
      calFailed();
    }
  }
}

void Positioner::startCentering(){
  if(mark.isActive()){
    //we might have first edge, might not.... until we figure that out (which we can)
    h.oming=Homage::Backward2Start;
    relmoveto(-c.h.widest);//across must include total windup+hysteresis
  } else {
    if(s.hasMark()){
      if(lastKnownIndex!=s.desiredIndex){
        //we fell short of the mark if going forward, or past it if going backward, either way we need both edges of interest
        h.oming=Homage::Forward2Fall;
        relmoveto(c.h.widest);
      } else {
        h.oming=Homage::Backward2Start;
        relmoveto(-c.h.widest);//across must include total windup+hysteresis
      }
    }
  }
}

bool Positioner::onMotionCompleted(bool normal /*else homed*/){//#ISR stepper motor's
  if(s.desiredIndex==-1){//if in diagnostic mode
    killProcedures();//else we quit looking for the commands that get us going again.
    return false;//do nothing
  }

  if(flagged(sizing)){
    onSizingComplete();
    return false;//there will be a move that will get us back here
  }

  if(flagged(zeroing)){
    lastKnownIndex=s.simpleHome()?1:s.g.numIndexes;
    //    s.setDesiredIndex(lastKnownIndex);
    m.setLocation(s.g.stepFor(lastKnownIndex));
    startCentering();
    return false;//confirm centered after what often is a full rotation.
  }

  if(normal){//return whether to center on mark
    if(s.hasMark()){
      startCentering();
      return false;//not done yet
    } else {
      return true;
    }
  } else {//just centered, return whether motion is really completed
    if(s.hasMark()){//exclude some diagnostic cases
      if(s.simpleHome()){
        m.setLocation(0);//filter wheel
      }
#if TrustMarks
      int width=motor.mark.netwidth();
      int skew=motor.mark.netskew();
      lastKnownIndex= s.g.indexFor(width);//compute index from mark's width
      loc[lastKnownIndex].step=motor.m.actualStep;
      loc[lastKnownIndex].markWidth=width;
#endif
    }
    return true;
  }
}

void Positioner::moveCompleted(){
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
void Positioner::onDone(void){ // on step done
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
        onMark(/*m.direction,*/inslot);
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


void Ramper::start(void){
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

int Positioner::circularize(int &step){
  return step=modulus(step,circularity);
}

void Positioner::absmoveto(int desired){//#ISR as well as not.
  if(desired<0){
    desired+=circularity;//but don't do full modulus as that interferes with the initial full scan.
  }
  if(!sizing && desired>circularity){
    desired-=circularity;
  }
  IRQLOCK(irq); //changes things the isr references.
  if(changed(m.desiredStep, desired)) {
    m.completed = false;//covers timing gap from command until next doLogic() invocation.
    trace(m);
  }
  beRunning(1);//# this is needed to unfreeze us after a calFailed.
}

void Positioner::relmoveto(int desired){
  absmoveto(m.actualStep+desired);
}

void Positioner::killProcedures(){
  sizing=zeroing=0;
  h.oming=Homage::NotHoming;
}

/** redefine where we are, but don't move*/
void Positioner::setLocation(int arg){
  m.setLocation(arg);
}

void Positioner::setCircularity(unsigned cyclelength){
  if(cyclelength<=2){//2 allows us to use stepper as a relay drive.
    wtf(4100);
    return;
  }
  circularity = cyclelength; //#converts to signed.
  mark.cyclelength=circularity;//cache
}

/**any time a control might have changed ... or any interrupt which includes system timer tick*/
void Positioner::doLogic( ){
  if(suppressedForDebug){
    return;
  }
  if(c.h.notConfigured()){
    return;
  }
  if(!c.v.isValid()){
    return;
  }
  if(calibrating()){
    return; //command processing trashes these
  }
  if(h.inProgress(false)){//if actively homing
    return; //command processing trashes centering
  }
  IRQLOCK(irq); //changes things the isr references.

  if(c.v.wasModified()){
    r.apply(c.v, *this);//convert Hz info ticks per step.
  }

  if(s.g.wasModified()){//tray geometry
    setCircularity(s.g.stepsPerCycle);//cache runtime expensive access
    killProcedures();
    lastKnownIndex=0;//which means we don't know where we are at all.
    setLocation(0);//want to move forward during sizing to match centering's direction
    EraseThing(loc);//for ease of debug

    if(s.g.isViable(true)){//tray installed
      sizing=true;
      absmoveto(s.g.stepsPerCycle+c.h.widest);//once around and a mark's worth for windup.
      //beRunning(1);
    } else {//not installed or not configured
      calFailed();//includes turning off isr
      output = 0b1111;//all coils freely spinning, but remember physical phase to reduce jerk when power is restored.
    }
    return;//4ezdebug
  }

  if(s.g.isViable() && s.wasModified()){//index position changed
    if(s.desiredIndex>0){
      absmoveto(s.desiredStep());
    } else if(s.desiredIndex==0){//diags feature
      absmoveto(0); //not necessarily the same as index position 1, but usually so.
      c.retrigger(1);//do a recentering even if already at 0.
    } //else ignore s, so that low level diagnostics prevail.
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
    if(s.g.isViable(false/*want to run when in diagnostics*/)){
      if(!inMotion()&& mark.isActive()){//if in a slot
        startCentering();//re-center, to deal with change of offset parameter.
        return;
      }
    }
  }

} /* doLogic */

bool Positioner::atLocation(u32 step){
  return !inMotion()&&h.hasHomed&& u32(m.actualStep)==step;
}

Positioner::Positioner(PositionerSettings &s,const Port&port, unsigned lsbit, unsigned timerLuno, const Pin *pin,PulseInput &index): //
  PeriodicInterrupter(timerLuno),
  s(s), //
  c(s.c), //
  output(2/*gp2MHz*/,port, lsbit, lsbit + 3), //
  mark(index),//
  powerPin(pin){
  beRunning(0); //coz debugger doesn't do a clean start.
  //don't need to lock as construction is static.
  suppressedForDebug=false;//only settable via a debugger
  phasor=0;
  lastKnownIndex = 0;
  circularity = 0;
  killProcedures();
  power(0); //power down ASAP.
  output = 0b1111;//all coils freely spinning.
}

void Positioner::init(void){
  setPrescaleFor(50000); //divisor needed to get the slowness we may need while still having nice decimal roundoff on values.
  m.init();
  output = phaseTable[phasor=0];//puts coils in known state.
  //leave isr dead until we have some live configuration.
}

void Positioner::report(PositionerReport &r){
  r.motionState=  inMotion()?1:(h.hasHomed?2:(irq.isEnabled()?0:-1));
  r.lastPosition=lastKnownIndex;
  r.targetPosition=s.desiredIndex;
}

void Positioner::reportMore(MotorReport &r){
  IRQLOCK(irq); //to ensure coherence
  r.motionCode= h.inProgress(0)?2:(inMotion()?1:0);
  r.homingStage=h.oming;
  r.hasHomed=h.hasHomed;
  r.location= m.actualStep;
  r.target= m.desiredStep;
  mark.markReport(r.markReport);
}

//end of file
