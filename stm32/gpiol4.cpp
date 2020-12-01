#pragma clang diagnostic push
//this inspection has some stupid variations, like requiring unsigned shift for unsigned integer.
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

//F4 has significantly different GPIO configuration than F1

#include "gpiol4.h"
#include "bitbasher.h"

// priority must be such that these get created before any application objects, +5 make it be after clocks.
#define DefinePort(letter)  {*#letter}


//the above macro is why people hate C. The '*' picks out the first letter of the string made by # letter, since the preprocessor insisted on honoring single ticks while parsing the #defined text.
InitStep(InitHardware + 5)
constexpr const Port Ports[]={
  DefinePort(A),
  DefinePort(B),
  DefinePort(C),
  DefinePort(D),
  DefinePort(E),
  DefinePort(F),
  DefinePort(G),
  DefinePort(H),
  //todo: conditional compile on DEVICE
  //DefinePort(I),
  //DefinePort(J),
};

constexpr Port::Field::Field(const Port &port, unsigned lsb, unsigned msb)
  : idr(port.registerAddress(0x10)), at(port.registerAddress(0x18)),  //bsrr "bit set/reset register"
  lsb(lsb), mask(fieldMask(msb, lsb) | fieldMask(msb, lsb) << 16), port(port) {
  /* empty */
}


void Port::Field::operator^=(unsigned value) const {
  return *this = (value ^ *this);  // uses operator = and operator cast uint16_t.
}

/** configure the given pin.   */
void Port::configure(unsigned bitnum, const PinOptions &c) const {
  if (!isEnabled()) { // deferred init, so we don't have to sequence init routines, and so we can statically create objects without wasting power if they aren't needed.
    init(); // must have the whole port running before we can modify a config of any pin.
  }
  //2 bits from dir into offset 0
  field(0x00, bitnum * 2, 2) = c.dir;

  //1 bit "is open drain" into offset 4 from UDFO==O
  bit(0x04, bitnum) = (c.UDFO == PinOptions::OpenDrain);

  //2 bits from slew into offset 8
  field(0x08, bitnum * 2, 2) = c.slew;

  //2 bits from UDFO into offset 12  F:0 U:1 O:1 D:2  (O goes to OD register and we pull up here)
  field(0x0C, bitnum * 2, 2) = c.UDFO >= PinOptions::Up ? 1 : (c.UDFO << 1);
  //alt function select is at offset 32, 4 bits each.
  field((bitnum & 8) ? 0x24 : 0x20, (bitnum & 7) * 4, 4) = c.altcode;
}

void Port::forAdc(unsigned int bitnum) const {
  configure(bitnum, PinOptions(PinOptions::analog, PinOptions::Slew::slow, PinOptions::Float));
}
//
//const Pin &Pin::AI() const {
//  port.forAdc(lsb);
//  return *this;
//}

const Pin &Pin::DI(PinOptions::Puller UDF) const {  // default Down as that is what meters will do.
  port.configure(lsb, PinOptions(PinOptions::input, PinOptions::Slew::slow, UDF));
  return *this;
}

///** configure pin as alt function output*/
//const Pin &Pin::FN(unsigned nibble, PinOptions::Slew slew, PinOptions::Puller UDF) const {
//  port.configure(lsb, PinOptions(PinOptions::function, slew, UDF, nibble));
//  return *this;
//}

const Pin &Pin::DO(PinOptions::Slew slew, PinOptions::Puller UDFO) const {
  port.configure(lsb, PinOptions(PinOptions::output, slew, UDFO));
  return *this;
}

//////////////////////////////////

#pragma clang diagnostic pop


