#include "timer.h"
#include "minimath.h"

#include "clocks.h"

#define PSC dcb[20]
#define ARR dcb[22]

//construction aid
struct TimerConstant {
  int apb;
  int slot;
  int irq;
//  enum DebugControlBit stopper;
};

static const TimerConstant T[] = {
  { 0, 0, 0 }, //spacer so that we can textually use st's 1-based labels.
#if DEVICE==103

  { 2, 11, 27 }, //CC, see also 25,26 ...
  { 1, 0, 28 },
  { 1, 1, 29 },
  { 1, 2, 30 },
  { 1, 3, 50 },
  { 1, 4, 54 },
  { 1, 5, 55 },
  { 2, 13, 44 },
#elif DEVICE==407
//stop bit is apb*32+slot
  { 2, 0, 27 }, //just CC
  { 1, 0, 28 },
  { 1, 1, 29 },
  { 1, 2, 30 },
  { 1, 3, 50 },
  { 1, 4, 54 },//shared with DAC
  { 1, 5, 55 },
  { 2, 1, 46 },//just cc

  { 2, 16, 24 },//#9 and T1 break
  { 2, 17, 25 },//#10 and T1 update
  { 2, 18, 26 },//#11 and T1 trigger
  { 1, 6, 43 },//#12 and T8 break
  { 1, 7, 44 },//#13 and T8 update
  { 1, 8, 45 },//#14 and T8 trigger
#endif
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

unsigned  Timer::baseRate(void) const {
  Hertz  apbRate = apb.getClockRate();
  Hertz  ahbRate = clockRate(0);

  return (ahbRate == apbRate) ? apbRate : apbRate *= 2;//#?F4?
}

unsigned  Timer::ticksForMillis(unsigned  ms) const {
  return quanta(ms * baseRate(), 1000 * (1 + PSC));
}

unsigned  Timer::ticksForMicros(unsigned  us) const {
  return quanta(us * baseRate(), 1000000 * (1 + PSC));
}

unsigned  Timer::ticksForHz(double Hz) const {
  return quanta(baseRate(), Hz * (1 + PSC));
}

double Timer::secondsInTicks(unsigned  ticks) const{
  return ratio(double(baseRate()),ticks*(1+PSC));
}

/**sets the prescalar to generate the given hz.*/
void Timer::setPrescaleFor(double hz) const {
  PSC = static_cast <u16> ( ratio( baseRate(), hz)) - 1; //e.g. 36MHz/10kHz = 35999
  //if we don't force an update cycle then we are at the mercy of other operations to allow an update event to occur. In onePulseMode that seems to never happen.
  b->fake_update = 1; //UG: an auto clearing bit.  UEV
}

void Timer::setCycler(unsigned  ticks) const {
  ARR = ticks - 1;//
}

unsigned  Timer:: getCycler() const {
  return unsigned (ARR) + 1; //#cast required
}

double Timer::getHz()const {
  return secondsInTicks(getCycler());
}


////////////////////////////
PeriodicInterrupter::PeriodicInterrupter(unsigned stLuno) : Timer(stLuno){
  beRunning(0);//for soft reset to match hard reset.
}

void PeriodicInterrupter::beRunning(bool on){ //can't const as interrupts are manipulated
  Interrupts(on); //raw interrupt is always on, UIE interrupt is only mask we twiddle.
  UIE(on);
  Timer::beRunning(on);
}

void PeriodicInterrupter::restart(unsigned  ticks){ //can't const as interrupts are manipulated
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

bool CCUnit::amDual() const {
  return timer.isAdvanced() && zluno < 3; //3 channels of advanced timers are dual output
}

void CCUnit::setmode(u8 cc) const {
  u16&pair(timer.dcb[12 + (2 * (zluno >= 2))]); //damned 16 bit access is painful

  if(zluno & 1) { //odd members are high half
    pair = (pair & ~0xFF00) | (cc << 8);
  } else {
    pair = (pair & ~0xFF) | cc;
  }
}

Monostable::Monostable(unsigned stLuno):Timer(stLuno){
  beRunning(0); //just in case someone fools with the base class
  Interrupts(1);
  UIE(1);
}

void Monostable::setPulseWidth(unsigned ticks){
  Timer::setCycler(ticks);  
}

void Monostable::setPulseMicros(unsigned micros){
  if(micros <= 0){
    micros=1;  //we do not know if zero will pulse here
  }
  setPulseWidth(Timer::ticksForMicros(micros));
}

void Monostable::retrigger(){
  startRunning();
}


//end of file
