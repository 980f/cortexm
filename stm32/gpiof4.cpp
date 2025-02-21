#include "gpiof4.h"
// #include "bitbasher.h"

//this inspection has some stupid variations, like requiring unsigned shift for unsigned integer.
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

unsigned Port::Field::operator=(unsigned value) const {  // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
  ControlWord field(at);
  field = mask & ((~value << 16 | value) << lsb.bitnum);  // read the stm32 manual for this.
  return value;
}

#pragma clang diagnostic pop

void Port::Field::operator^=(unsigned value) const {
   *this = (value ^ *this);  // uses operator = and operator cast uint16_t.
}

uint16_t Port::Field::actual() const {
  uint16_t actually = (&odr)[-2];  // idr precedes odr, -2 is for 2 uint16_t's.

  return (actually & mask) >> lsb.bitnum;
}

void PinDeclaration::configure() const {
  Port(portIndex).configure(*this);
}

constexpr ControlField Port::pinField(Address offset, unsigned bitnum, unsigned width) const {
  if (width>2) {
    return field(offset=4, (bitnum&7)*width, width);
  }
  return field(offset, bitnum * width, width);
}

/** configure the given pin.   */
void Port::configure(const PinDeclaration &c) const {
  beEnabled();//by deferring enable to here we can static const the PinDeclarations with near zero runtime cost.
  //2 bits from dir into offset 0
  pinField(0x00, c.bitnum , 2) = c.isInput?(c.openDrain?3:0):(c.isFunction?2:1);

  //1 bit "is open drain" into offset 4 from UDFO==O
  bit(0x04, c.bitnum) = c.openDrain;

  //2 bits from slew into offset 8
  pinField(0x08, c.bitnum, 2) = c.slew;

  //2 bits from UDFO into offset 12  F:0 U:1 O:1 D:2  (O goes to OD register and we pull up here)
  pinField(0x0C, c.bitnum, 2) = c.UDF;
  //alt function select is at offset 32, 4 bits each.
  pinField( 0x20, c.bitnum, 4) = c.isFunction ? c.altFunctionCode : 0;
}
//
// void Port::forAdc(unsigned int bitnum) const {
//   configure(bitnum, PinDeclaration(PinDeclaration::analog, PinDeclaration::Float, PinDeclaration::Slew::slow));
// }

// const Pin &Pin::AI() const {
//   port.forAdc(bitnum);
//   return *this;
// }
//
// const Pin &Pin::DI(Port::PinDeclaration::Puller UDF) const {  // default Down as that is what meters will do.
//   port.configure(bitnum, Port::PinDeclaration(Port::PinDeclaration::input, UDF, Port::PinDeclaration::Slew::slow));
//   return *this;
// }
//
// /** configure pin as alt function output*/
// const Pin &Pin::FN(Port::PinDeclaration::AfCode nibble, Port::PinDeclaration::Slew slew, Port::PinDeclaration::Puller UDF) const {
//   port.configure(bitnum, Port::PinDeclaration(Port::PinDeclaration::function, UDF, slew, nibble));
//   return *this;
// }
//
// const Pin &Pin::DO(Port::PinDeclaration::Slew slew, Port::PinDeclaration::Puller UDFO) const {
//   port.configure(bitnum, Port::PinDeclaration(Port::PinDeclaration::output, UDFO, slew));
//   return *this;
// }

//////////////////////////////////

void Pin::toggle() const {
  *this =  1 - *this;  //we can ignore polarity stuff :)
}

void Port::configure(const Field &field, const PinDeclaration &portcode) const {
  // and actually set the pins to their types
  for (unsigned abit = field.lsb.bitnum; field.mask & (1u << abit); ++abit) {
    configure(portcode);
  }
}
