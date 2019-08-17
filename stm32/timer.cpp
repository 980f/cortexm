#include "timer.h"
#include "minimath.h"
#include "afio.h"
#include "clocks.h"

#define PSC dcb[20]
#define ARR dcb[22]

//construction aid
struct TimerConstant {
  int apb;
  int slot;
  int irq;
  enum DebugControlBit stopper;
};

//todo: make #defined symbol for IRQ numbers for ObjectInterrupt macro. Need it as text, not const value.
static const TimerConstant T[] = {
  { 0, 0, 27, Timer1Stop }, //spacer so that we can textually use st's 1-based labels.
  { 2, 11, 25, Timer1Stop },
  { 1, 0, 28, Timer2Stop },
  { 1, 1, 29, Timer3Stop },
  { 1, 2, 30, Timer4Stop },
  { 1, 3, 50, Timer5Stop },
  { 1, 4, 54, Timer6Stop },
  { 1, 5, 55, Timer7Stop },
  { 2, 13, 44, Timer8Stop },
};

Timer::Timer(unsigned stLuno): apb(T[stLuno].apb, T[stLuno].slot), irq(T[stLuno].irq){
  dcb = reinterpret_cast <TIMER_DCB> (apb.blockAddress);
  b = reinterpret_cast <TIMER_BAND *> (apb.bandAddress);
  luno = stLuno;
  apb.init(); //for most extensions this is the only time we do the init.
}

/** setting up the actual pin must be done elsewhere*/
void Timer::configureCountExternalInput(enum Timer::ExternalInputOption which, unsigned filter) const {
  apb.init(); //wipe all previous settings
  //47 = t1edge, external mode 1. ?:1 gives a 1 for 0 or 1, a 2 for 2.
  dcb[4] = 0x0047 + ((which ? : 1) << 4); //smcr
  if(which == Xor) {
    b->in1Xored = 1;
  }
  b->updateIE = 1; //enabling just the update interrupt for carry-outs
  dcb[12] |= filter << (which == CH2 ? 12 : 4);
} /* configureCountExternalInput */

u32 Timer::baseRate(void) const {
  u32 apbRate = apb.getClockRate();
  u32 ahbRate = clockRate(0);

  return (ahbRate == apbRate) ? apbRate : apbRate *= 2;
}

u32 Timer::ticksForMillis(u32 ms) const {
  return quanta(ms * baseRate(), 1000 * (1 + PSC));
}

u32 Timer::ticksForHz(double Hz) const {
  return quanta(baseRate(), Hz * (1 + PSC));
}

double Timer::secondsInTicks(u32 ticks) const{
  return ratio(double(baseRate()),ticks*(1+PSC));
}

/**sets the prescalar to generate the given hz.*/
void Timer::setPrescaleFor(double hz) const {
  PSC = static_cast <u16> ( ratio( baseRate(), hz)) - 1; //e.g. 36MHz/10kHz = 35999
  //if we don't force an update cycle then we are at the mercy of other operations to allow an update event to occur. In onePulseMode that seems to never happen.
  b->fake_update = 1; //UG: an auto clearing bit.  UEV
}

void Timer::setCycler(u32 ticks) const {
  ARR = ticks - 1;//
}

u32 Timer:: getCycler() const {
  return u32(ARR) + 1; //#cast required
}

double Timer::getHz()const {
  return secondsInTicks(getCycler());
}

void PeriodicInterrupter::beRunning(bool on){ //can't const as interrupts are manipulated
  Interrupts(on); //raw interrupt is always on, UIP interrupt is only mask we twiddle.
  UIE(on);
  Timer::beRunning(on);
}

void PeriodicInterrupter::restart(u32 ticks){ //can't const as interrupts are manipulated
  setCycler(ticks);
  beRunning(true);
}

void PeriodicInterrupter::restartHz(double hz){ //can't const as interrupts are manipulated
  restart(ticksForHz(hz));
}

DelayTimer::DelayTimer(int luno, int ccluno): Timer(luno), cc(*this, ccluno){}

void DelayTimer::init(int hzResolution) const {
  apb.init();
  //the apb.init sets up free running upcounter.
  setPrescaleFor(hzResolution);
}

const CCUnit& DelayTimer::setDelay(int ticks) const {
  cc.setmode(0x10); //set on match, only clear by software
  cc.IE(1);
  cc.setTicks(ticks);
  return cc;
}
////////////////////////////
CCUnit::CCUnit (const Timer&_timer, unsigned _ccluno): timer(_timer), zluno(_ccluno - 1){
  //nothing to do here
}

Pin CCUnit::pin(unsigned alt, bool complementaryOutput ) const {
  switch(timer.luno) {
  case 1:
    if(complementaryOutput) {
      //todo:3 ignoring alt for a bit:
      return Pin(PB, 13 + zluno);
    } else {
      return alt == 3 ? Pin(PE, 9 + 2 * zluno) : Pin(PA, 8 + zluno); //todo:3 alt formula fails for zlun0==3
    }
  case 2:
    switch(zluno){
    case 0: return Pin(PA,bitFrom(alt,0)?15:0);
    case 1: return bitFrom(alt,0)?Pin(PB,3):Pin(PA,1);
    case 2: return bitFrom(alt,1)?Pin(PB,10):Pin(PA,2);
    case 3: return bitFrom(alt,1)?Pin(PB,11):Pin(PA,3);
    } break;
  case 3:   //todo:3 ignoring alt for a bit:
    return zluno < 2 ? Pin(PA, 4 + zluno) : Pin(PB, zluno - 2);
  case 4:
    return alt ? Pin(PD, 10 + zluno) : Pin(PB, 4 + zluno);
    //no timer 5,6,7, or 8 in parts of immediate interest.
  } /* switch */
  return Pin(Port('Z'), 0); //should blow royally
} /* pin */

bool CCUnit::amDual(void) const {
  return timer.isAdvanced() && zluno < 3; //3 channels of advanced timers are dual output
}
//set polarity
//force on or off using cc config rather than gpio.
//if timer is #1 or #8 then there are more bits:
void CCUnit::takePin(unsigned alt,bool inverted) const { //todo:3 options to only take one of a pair.
  CCER&myccer = ccer();

  switch(timer.luno){
  case 1:theAfioManager.remap.tim1=alt; break;
  case 2:theAfioManager.remap.tim2=alt; break;
  case 3:theAfioManager.remap.tim3=alt; break;
  case 4:theAfioManager.remap.tim4=alt; break;
  case 5:theAfioManager.remap.tim5=alt; break;
    //no remaps for other timers, could wtf if alt!=0
  }
  theAfioManager.remap.update();

  pin(alt, 0).FN();
  if(amDual()) { //todo:3 'and Take complementary pin as well'
    pin(alt, 1).FN();
    myccer.NE = 0; //+adv 1..3
    myccer.NP = 0; //+adv 1..3
    timer.b->MOE = 1;
    //don't care about idle state...
  }
  myccer.Enabled = 1;
  myccer.Inverted = inverted;
} /* takePin */

void CCUnit::setmode(u8 cc) const {
  u16&pair(timer.dcb[12 + (2 * (zluno >= 2))]); //damned 16 bit access is painful

  if(zluno & 1) { //odd members are high half
    pair = (pair & ~0xFF00) | (cc << 8);
  } else {
    pair = (pair & ~0xFF) | cc;
  }
}

//end of file
