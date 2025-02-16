#include "gpiof4.h"
#include "bitbasher.h"

//this inspection has some stupid variations, like requiring unsigned shift for unsigned integer.
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

constexpr Port::Field::Field(const Port &port, unsigned lsb, unsigned msb) :
  lsb(lsb)
  , mask(fieldMask(msb, lsb) | fieldMask(msb, lsb) << 16)
  , odr(port.registerAddress(0x14))
  , at(port.registerAddress(0x18))  //bsrr "bit set/reset register"
  , port(port) {
  /* empty */
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

void Port::Field::operator=(unsigned value) const {  // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
  ControlWord field(at);
  field = mask & (((((~value) << 16) | value)) << lsb);  // read the stm32 manual for this.
}

#pragma clang diagnostic pop

void Port::Field::operator^=(unsigned value) const {
  return *this = (value ^ *this);  // uses operator = and operator cast uint16_t.
}

uint16_t Port::Field::actual() const {
  uint16_t actually = (&odr)[-2];  // idr precedes odr, -2 is for 2 uint16_t's.

  return (actually & mask) >> lsb;
}

/** configure the given pin.   */
void Port::configure(unsigned bitnum, const PinOptions &c) const {
  beEnabled();
  //2 bits from dir into offset 0
  field(0x00, bitnum * 2, 2) = c.dir;

  //1 bit "is open drain" into offset 4 from UDFO==O
  bit(0x04, bitnum) = (c.UDFO == PinOptions::OpenDrain);

  //2 bits from slew into offset 8
  field(0x08, bitnum * 2, 2) = c.slew;

  //2 bits from UDFO into offset 12  F:0 U:1 O:1 D:2  (O goes to OD register and we pull up here)
  field(0x0C, bitnum * 2, 2) = c.UDFO >= PinOptions::Up ? 1 : (c.UDFO << 1);
  //alt function select is at offset 32, 4 bits each.
  field((bitnum > 7) ? 0x24 : 0x20, (bitnum & 7) * 4, 4) = c.altcode;
}

void Port::forAdc(unsigned int bitnum) const {
  configure(bitnum, PinOptions(PinOptions::analog, PinOptions::Float, PinOptions::Slew::slow));
}

const Pin &Pin::AI() const {
  port.forAdc(bitnum);
  return *this;
}

const Pin &Pin::DI(Port::PinOptions::Puller UDF) const {  // default Down as that is what meters will do.
  port.configure(bitnum, Port::PinOptions(Port::PinOptions::input, UDF, Port::PinOptions::Slew::slow));
  return *this;
}

/** configure pin as alt function output*/
const Pin &Pin::FN(Port::PinOptions::Altfunc nibble, Port::PinOptions::Slew slew, Port::PinOptions::Puller UDF) const {
  port.configure(bitnum, Port::PinOptions(Port::PinOptions::function, UDF, slew, nibble));
  return *this;
}

const Pin &Pin::DO(Port::PinOptions::Slew slew, Port::PinOptions::Puller UDFO) const {
  port.configure(bitnum, Port::PinOptions(Port::PinOptions::output, UDFO, slew));
  return *this;
}

//////////////////////////////////

void OutputPin::toggle() const {
  pin = 1 - pin;  //we can ignore polarity stuff :)
}

void Port::configure(const Port::Field &field, const Port::PinOptions &portcode) const {
  // and actually set the pins to their types
  for (unsigned abit = field.lsb; field.mask & (1u << abit); ++abit) {
    configure(abit, portcode);
  }
}
