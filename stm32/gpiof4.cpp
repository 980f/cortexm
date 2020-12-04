#pragma clang diagnostic push
//this inspection has some stupid variations, like requiring unsigned shift for unsigned integer.
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

//F4 has significantly different GPIO configuration than F1

#include "gpiof4.h"
#include "bitbasher.h"

// priority must be such that these get created before any application objects, +5 make it be after clocks.
#define DefinePort(letter) const Port P##letter InitStep(InitHardware + 5)(*#letter)
//the above macro is why people hate C. The '*' picks out the first letter of the string made by # letter, since the preprocessor insisted on honoring single ticks while parsing the #defined text.

DefinePort(A);
DefinePort(B);
DefinePort(C);
DefinePort(D);
DefinePort(E);
DefinePort(F);
DefinePort(G);
DefinePort(H);
DefinePort(I);
DefinePort(J);

constexpr Port::Field::Field(const Port &port, unsigned lsb, unsigned msb)
  : odr(port.registerAddress(0x14)),
    at(port.registerAddress(0x18)),  //bsrr "bit set/reset register"
    lsb(lsb),
    mask(fieldMask(msb, lsb) | fieldMask(msb, lsb) << 16),
    port(port) {
  /* empty */
}

void Port::Field::operator=(unsigned value) const {  // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
  ControlWord {at} = mask & (((((~value) << 16) | value)) << lsb);  // read the stm32 manual for this.
}

Port::Field::operator u16() const {
  return (odr & mask) >> lsb;
}

void Port::Field::operator^=(unsigned value) const {
  return *this = (value ^ *this);  // uses operator = and operator cast u16.
}

u16 Port::Field::actual() const {
  u16 actually = (&odr)[-2];  // idr precedes odr, -2 is for 2 u16's.

  return (actually & mask) >> lsb;
}

static unsigned udfo2code(char UDFO) {
  switch (UDFO) {
    case 'U':
      return 1;
    case 'D':
      return 2;
    default:
    case 'F':
      return 0;
    case 'O':
      return 1;//plus other stuff happens
  }
}

/** configure the given pin.   */
void Port::configure(unsigned bitnum, const PinOptions &c) const {
  if (!isEnabled()) { // deferred init, so we don't have to sequence init routines, and so we can statically create objects without wasting power if they aren't needed.
    init(); // must have the whole port running before we can modify a config of any pin.
  }
  //2 bits from dir into offset 0
  field(0x00, bitnum * 2, 2) = c.dir;

  //1 bit "is open drain" into offset 4 from UDFO=='O'
  bit(registerAddress(0x04), bitnum) = c.UDFO == 'O';

  //2 bits from slew into offset 8
  field(0x08, bitnum * 2, 2) = c.slew;

  //2 bits from UDFO into offset 12  F:0 U:1 D:2  (O goes to OD register and we pull up here)  F=0x46 U=0x85 D=0x44 Oh=0x79
  unsigned code = udfo2code(c.UDFO);//failed: ((c.UDFO=='D')?2:0) + (c.UDFO&1);
  field(0x0C, bitnum * 2, 2) = code;
  //alt function select is at offset 32, 4 bits each.
  field((bitnum >= 8) ? 0x24 : 0x20, (bitnum & 7) * 4, 4) = c.altcode;
}

void Port::forAdc(unsigned int bitnum) const {
  configure(bitnum, PinOptions(PinOptions::analog, PinOptions::Slew::slow, 'F'));
}

const Pin &Pin::AI() const {
  port.forAdc(bitnum);
  return *this;
}

const Pin &Pin::DI(char UDF) const {  // default Down as that is what meters will do.
  port.configure(bitnum, PinOptions(PinOptions::input, PinOptions::Slew::slow, UDF));
  return *this;
}

/** configure pin as alt function output*/
const Pin &Pin::FN(unsigned nibble, PinOptions::Slew slew, char UDFO) const {
  port.configure(bitnum, PinOptions(PinOptions::function, slew, UDFO,nibble));
  return *this;
}

//////////////////////////////////

void OutputPin::toggle() const {
  pin = 1 - pin;  //we can ignore polarity stuff :)
}

/////////////////////////////////

#pragma clang diagnostic pop
