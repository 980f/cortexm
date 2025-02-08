#pragma clang diagnostic push
//this inspection has some stupid variations, like requiring unsigned shift for unsigned integer.
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#include <gpio.h>

#include "peripheralband.h" //deprecated bandFor
#include "bitbasher.h"

//init is now on demand in pin instantiation, so init order doesn't matter.
__attribute((used, section(".rodata.constinit" )))
const Port PA('A');
constexpr Port PB('B');
const Port PC('C');
const Port PD('D');
const Port PE('E');


// todo:3 use device define to add ports up to G.

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

///////////////////////////
//
//void Pin::configureAs(unsigned int code) const {
//  port.configure(bitnum, code);
//}

//constexpr void Pin::output(unsigned int code, Portcode::Slew  slew, bool openDrain) const {
//  code |= openDrain << 2;
//  switch(slew) {
//  default: // on any errors be a slow output
//  case Portcode::Slew::slow: code |= 2; break;
//  case Portcode::Slew::medium: code |= 1; break;
//  case Portcode::Slew::fast: code |= 3; break;
//  }
//  configureAs(code);
//}

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

//ControlWord Pin::highDriver() const {
//  Address confword=port.registerAddress((bitnum >= 8) ? 4 : 0);
//  unsigned bitoff = (bitnum & 7) * 4 + 2;
//
//  return ControlWord(confword, bitoff);
//}



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

/////////////////////////////////

bool Port::isOutput(unsigned pincode) {
  return (pincode & 3U) != 0;//if so then code is Alt/Open
}

// constexpr Port::Port(char letter) : APBdevice(BusNumber::AHB1, 2 + unsigned(letter - 'A')) {}

//constexpr void Port::configure(unsigned bitnum, unsigned code) const {
//  if(! isEnabled()) { // deferred init, so we don't have to sequence init routines, and so we can statically create objects without wasting power if they aren't needed.
//    init(); // must have the whole port running before we can modify a config of any pin.
//  }
//  ControlField confword(registerAddress(bitnum & 8 ? 4 : 0), (bitnum & 7) << 2, 4);// &7:modulo 8, number of conf blocks in a 32 bit word.; 4 bits each block
//  confword= code;
//}

#pragma clang diagnostic pop
