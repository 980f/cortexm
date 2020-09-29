#include "timer.h"
#include "minimath.h"

#include "clocks.h"

#define PSC word(0x28)
#define ARR word(0x2C)



/** setting up the actual pin must be done elsewhere*/
void Timer::configureCountExternalInput(enum Timer::ExternalInputOption which, unsigned filter) const {
  APBdevice::init(); //wipe all previous settings
  //47 = t1edge, external mode 1. ?:1 gives a 1 for 0 or 1, a 2 for 2.
  word(0x08) = 0x0047 + ((which ?: 1) << 4); //smcr
  if (which == Xor) {
//todo: restore    bit() = 1;
  }
  bit(0x0C, 0) = 1; //enabling just the update interrupt for carry-outs
  word(0x18) |= filter << (which == CH2 ? 12 : 4);//todo: verify the 0x18
} /* configureCountExternalInput */

Hertz Timer::baseRate() const {
  Hertz apbRate = APBdevice::getClockRate();
  Hertz ahbRate = clockRate(0);
  return (ahbRate == apbRate) ? apbRate : apbRate * 2;//#?F4?
}

unsigned Timer::ticksForMillis(unsigned ms) const {
  return quanta(ms * baseRate(), 1000 * (1 + PSC));
}

unsigned Timer::ticksForMicros(unsigned us) const {
  return quanta(us * baseRate(), 1000000 * (1 + PSC));
}

unsigned Timer::ticksForHz(double Hz) const {
  return quanta(baseRate(), Hz * (1 + PSC));
}

double Timer::secondsInTicks(unsigned ticks) const {
  return ratio(double(baseRate()), ticks * (1 + PSC));
}

/**sets the prescalar to generate the given hz.*/
void Timer::setPrescaleFor(double hz) const {
  PSC = static_cast <u16> ( ratio(baseRate(), hz)) - 1; //e.g. For F103: 36MHz/10kHz = 35999
  //if we don't force an update cycle then we are at the mercy of other operations to allow an update event to occur. In onePulseMode that seems to never happen.
  update();
}

void Timer::setCycler(unsigned ticks) const {
  ARR = ticks - 1;//
}

unsigned Timer::getCycler() const {
  return unsigned(ARR) + 1; //#cast required
}

double Timer::getHz() const {
  return secondsInTicks(getCycler());
}

void Timer::update() const {
  bit(0x14, 0) = 1; //UG: an auto clearing bit.
}
void Timer::init() const {
  APBdevice::init();
}

////////////////////////////
PeriodicInterrupter::PeriodicInterrupter(unsigned stLuno) : Timer(stLuno) {
  //removed so that we can constexpr beRunning(0);//for soft reset to match hard reset.
}

void PeriodicInterrupter::beRunning(bool on)const {
  Interrupts(on); //raw interrupt is always on, UIE interrupt is only mask we twiddle.
  UIE(on);
  Timer::beRunning(on);
}

void PeriodicInterrupter::restart(unsigned ticks)const {
  setCycler(ticks);
  beRunning(true);
}

void PeriodicInterrupter::restartHz(double hz) const{
  restart(ticksForHz(hz));
}

///////////////////////////////////////////////////////////
void DelayTimer::init(int hzResolution) const {
  APBdevice::init();  //this apb.init sets up free running upcounter.
  setPrescaleFor(hzResolution);
}

const CCUnit &DelayTimer::setDelay(int ticks) const {
  cc.setmode(0b0'001'00'00); //set on match, only clear by software
  cc.IE(1);
  cc.setTicks(ticks);
  return cc;
}
////////////////////////////

bool CCUnit::amDual() const {
  return timer.isAdvanced() && zluno < 3; //3 channels of advanced timers are dual output
}

/* mode bits
 *   for output:
 *    0bx ___ __ 00  clear enable
 *    0b_ abc __ 00  000 do nothing
 *                   001 set on match
 *                   010 clear on match
 *                   011 toggle on match
 *                   100 always off
 *                   101 always on
 *                   110 pwm: active early 'half'
 *                   111 pwm: active later 'half'
 *    0b_ ___ x_ 00  0=immediate effect, 1=pipelined
 *    0b_ ___ _X 00  if in pwm then 1 expedites output update on "trigger"
 * */
void CCUnit::setmode(u8 cc) const {
  u16 &pair = Ref<u16>(timer.registerAddress(0x18 + (2 * (zluno > 2)))); //damned mandatory 16 bit access is painful
  if (zluno & 1) { //odd members are high half
    pair = (pair & ~0xFF00) | (cc << 8);
  } else {
    pair = (pair & ~0xFF) | cc;
  }
}

void CCUnit::takePin(bool activehigh) const {
  // presume device pin has been already set for FN(thistimer)
  const unsigned int bitbase = zluno * 4;
  timer.bit(0x20, bitbase) = true;//take the bit
  timer.bit(0x20, bitbase + 1) = !activehigh;//0 is active high
}

void CCUnit::force(bool active) const {
  setmode(0b0'100'00'00 | (active << 4));
}

void Monostable::init() const {

  beRunning(0); //just in case someone fools with the base class
  Interrupts(1);
  UIE(1);
}

void Monostable::setPulseWidth(unsigned ticks) {
  Timer::setCycler(ticks);
}

void Monostable::setPulseMicros(unsigned micros) {
  if (micros <= 0) {
    micros = 1;  //we do not know if zero will pulse here
  }
  setPulseWidth(Timer::ticksForMicros(micros));
}

void Monostable::retrigger() {
  startRunning();
}


//end of file
