#pragma clang diagnostic push
//this inspection has some stupid variations, like requiring unsigned shift for unsigned integer.
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#include "gpio.h"

#include "peripheralband.h" 
#include "bitbasher.h"

const Port PA('A');
const Port PB('B');
const Port PC('C');
const Port PD('D');
const Port PE('E');
const Port PF('F');
const Port PG('G');

constexpr Port::Field::Field(const Port &port, unsigned lsb, unsigned msb) :
  odr(port.registerAddress(0x0C)),
  at(port.registerAddress(0x10)),
  lsb(lsb),
  mask(fieldMask(msb, lsb) | fieldMask(msb, lsb) << 16),
  port(port) {
  /* empty */
}

void Port::Field::operator=(unsigned value) const { // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
  ControlWord field(at);
  field = mask & (((((~value) << 16) | value)) << lsb); // read the stm32 manual for this.
}

Port::Field::operator u16() const {
  return (odr & mask) >> lsb;
}

void Port::Field::operator^=(unsigned value) const {
  return *this = (value ^ *this); // uses operator = and operator cast u16.
}

u16 Port::Field::actual() const {
  u16 actually = (&odr)[-2]; // idr precedes odr, -2 is for 2 u16's.

  return (actually & mask) >> lsb;
}

const Pin &Pin::AI() const {
  // ReSharper disable once CppExpressionWithoutSideEffects
  configureAs(0);
  return *this;
}

const Pin & Pin::DI(char UDF) const { // default Down as that is what meters will do.
  writer = bitFrom(UDF, 0); // ODR determines whether a pullup else a pulldown is connected ... this takes advantage of the ascii codes for U and D differing in the lsb.
  // NB: if the pin is already an output then the above line pulses current into it a moment before the next line turns off the driver. This is typically a good thing.
  return configureAs((UDF == 'F') ? 4 : 8); // ... this determines if the pin actually gets pulled.
}

const Pin& Pin::FN(Port::PinOptions::Slew slew, bool openDrain) const {
  return output(8, slew, openDrain);
}

//////////////////////////////////

InputPin::InputPin(const Pin &pin, char UDF, bool lowactive) : LogicalPin(pin, lowactive) {
  pin.DI(UDF);
}

InputPin::InputPin(const Pin &pin, bool lowactive) : InputPin(pin, lowactive ? 'U' : 'D', lowactive) {
  /*empty*/
}

//////////////////////////////////

void OutputPin::toggle() const {
  pin = 1 - pin;//we can ignore polarity stuff :)
}

bool Port::isOutput(unsigned pincode) {
  return (pincode & 3U) != 0;//if so then code is Alt/Open
}

#pragma clang diagnostic pop
