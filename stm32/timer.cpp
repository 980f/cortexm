#include "timer.h"

#include "clocks.h"

/* Add fields only as code needs to use them. The timers are so complex (or perhaps just poorly documented) that you should be reading the manual when trying to code something.
 * the base class has functions bit() field() and word() which take the values from the RMxxxx documentation.
 * The bit() uses the bitband mechanism.
 * The field() tends to shifting and masking.
 * The word() is for registers with a single value.
 */


/** setting up the actual pin must be done elsewhere*/
void Timer::configureCountExternalInput(enum Timer::ExternalInputOption which, unsigned filter) const {
  APBdevice::init(); //wipe all previous settings
  //47 = t1edge, external mode 1. ?:1 gives a 1 for 0 or 1, a 2 for 2.
  word(0x08) = 0x0047 | ((which ? : 1) << 4); //smcr
  if (which == Xor) {
//todo:M restore    bit(??) = 1;
  }
  UIE = true; //enabling just the update interrupt for carry-outs
  word(0x18) |= filter << (which == CH2 ? 12 : 4);//todo:M verify the 0x18
} /* configureCountExternalInput */

Hertz Timer::baseRate() const {
  Hertz apbRate = getClockRate();
  Hertz ahbRate = clockRate(AHB1);
  return (ahbRate == apbRate) ? apbRate : apbRate * 2;
}

unsigned Timer::ticksForMillis(unsigned ms) const {
  return ticksForSeconds(1e-3*ms);
}

unsigned Timer::ticksForMicros(unsigned us) const {
  return ticksForSeconds(1e-6*us);
}

unsigned Timer::ticksForHz(double Hz) const {
  return quanta(baseRate(), Hz * (1 + PSC));
}

float Timer::secondsInTicks(unsigned ticks) const {
  return ratio(float(baseRate()), ticks * (1 + PSC));
}

/**sets the prescalar to generate the given hz.*/
void Timer::setPrescaleFor(double hz) const {
<<<<<<< HEAD
  PSC = static_cast <u16> ( ratio( baseRate(), hz)) - 1; //e.g. 36MHz/10kHz = 35999
  //if we don't force an update cycle then we are at the mercy of other operations to allow an update event to occur. In onePulseMode that seems to never happen.
  b.fake_update = 1; //UG: an auto clearing bit.  UEV
=======
  unsigned int baserate = baseRate();
  float maxticks = ratio(baserate, hz);
  if (maxticks > 65535.0) {
    maxticks /= 65536.0F;
  }
  PSC = static_cast <u16> ( maxticks) - 1; //e.g. For F103: 36MHz/10kHz = 35999
>>>>>>> 38af48a193bdaa269537e1f7a37b0db25d5a1b03
}

void Timer::setCycler(unsigned ticks) const {
  ARR = ticks - 1;//
}

unsigned Timer::getCycler() const {
  return unsigned(ARR) + 1; //#cast required
}

float Timer::getHz() const {
  return secondsInTicks(getCycler());
}

void Timer::update() const {
  bit(0x14, 0) = 1; //UG: an auto clearing bit.
}

void Timer::init() const {
  APBdevice::init();
}

void Timer::startRunning() const {
  beRunning(false); //to ensure events are honored on very short counts when interrupted between clearing counts and clearing events.
  counter() = 0;
  clearEvents();
  beRunning(true);
}

unsigned Timer::ticksForSeconds(double secs) const {
  return ceilf(secs*(double(baseRate())/(1 + PSC)));
}

////////////////////////////
PeriodicInterrupter::PeriodicInterrupter(unsigned stLuno) : Timer(stLuno) {
  //removed so that we can constexpr beRunning(0);//for soft reset to match hard reset.
}

void PeriodicInterrupter::beRunning(bool on) const {
  Interrupts(on); //raw interrupt is always on, UIE interrupt is only mask we twiddle.
  UIE = on;
  if (on) {
    OPM = 0;//pulse train please.
  }
  Timer::beRunning(on);
}

void PeriodicInterrupter::restart(unsigned ticks) const {
  setCycler(ticks);
  update();//should be a 'kickme' on setCycler.
  beRunning(true);
}

void PeriodicInterrupter::restartHz(double hz) const {
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

<<<<<<< HEAD
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
    case 2:return bitFrom(alt,1)?Pin(PB,10):Pin(PA,2);
    case 3:return bitFrom(alt,1)?Pin(PB,11):Pin(PA,3);
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
  volatile CCER& myccer = ccer();

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
    timer.b.MOE = 1;
    //don't care about idle state...
  }
  myccer.Enabled = 1;
  myccer.Inverted = inverted;
} /* takePin */
=======
//////////////////////////////////////////////////////////
bool CCUnit::amDual() const {
  return timer.isAdvanced() && zluno < 3; //3 channels of advanced timers are dual output
}
>>>>>>> 38af48a193bdaa269537e1f7a37b0db25d5a1b03

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
<<<<<<< HEAD
  volatile u16 &pair(timer.dcb[12 + (2 * (zluno >= 2))]); //damned 16 bit access is painful

  if(zluno & 1) { //odd members are high half
=======
  u16 &pair = Ref<u16>(timer.registerAddress((zluno >= 2 ? 0x1C : 0x18))); //damned mandatory 16 bit access is painful
  if (zluno & 1) { //odd members are high half
>>>>>>> 38af48a193bdaa269537e1f7a37b0db25d5a1b03
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
  Timer::init();
  OPM = 1;
}

//////////////////////////////////////////////////////
void Monostable::setPulseWidth(unsigned ticks) const {
  if (ticks <= 0) {
    ticks = 1;  //we do not know if zero will pulse here
  }
  Timer::setCycler(ticks);
}

void Monostable::setPulseMicros(unsigned micros) const {
  setPulseWidth(ticksForMicros(micros));
}



//end of file
