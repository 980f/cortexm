#pragma clang diagnostic push
//this inspection has some stupid variations, like requiring unsigned shift for unsigned integer.
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

//F4 has significantly different GPIO configuration than F1

#include "gpiof4.h"
#include "peripheralband.h" //deprecated bandFor
#include "bitbanger.h"

// priority must be such that these get created before any application objects
#define DefinePort(letter) const Port P##letter InitStep(InitHardware)('##letter##');

DefinePort(a);
DefinePort(B);
DefinePort(C);
DefinePort(D);
DefinePort(E);
DefinePort(F);
DefinePort(G);
DefinePort(H);
DefinePort(I);
DefinePort(J);

constexpr Port::Field::Field(const Port &port, unsigned lsb, unsigned msb) :
  odr(port.registerAddress(0x14)),
  at(port.registerAddress(0x18)), //bssr
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
     port.configure(bitnum, PinOptions(PinOptions::analog, PinOptions::Slew::slow, 'F'));
    return *this;
}

const Pin & Pin::DI(char UDF) const { // default Down as that is what meters will do.
     port.configure(bitnum, PinOptions(PinOptions::input, PinOptions::Slew::slow, UDF));
    return *this;
}

/** configure pin as alt function output*/
  const Pin &Pin::FN(PinOptions::Slew slew , char UDFO) const{
    port.configure(bitnum, PinOptions(PinOptions::function, slew, UDFO));
    return *this;
  }

  /** set the alt feature, */
  const Pin &Pin::Alt(unsigned nibble) const {
    
  }
//////////////////////////////////

constexpr InputPin::InputPin(const Pin &pin, char UDF, bool lowactive) : LogicalPin(pin, lowactive) {
  pin.DI(UDF);
}

constexpr InputPin::InputPin(const Pin &pin, bool lowactive) : InputPin(pin, lowactive ? 'U' : 'D', lowactive) {
  /*empty*/
}

//////////////////////////////////


void OutputPin::toggle() const {
  pin = 1 - pin;//we can ignore polarity stuff :)
}

/////////////////////////////////
//
//bool Port::isOutput(unsigned pincode) {
//  return (pincode & 3U) != 0;//if so then code is Alt/Open
//}

constexpr Port::Port(char letter) : APBdevice(2, 2 + unsigned(letter - 'A')) {}

//constexpr void Port::configure(unsigned bitnum, unsigned code) const {
//  if(! isEnabled()) { // deferred init, so we don't have to sequence init routines, and so we can statically create objects without wasting power if they aren't needed.
//    init(); // must have the whole port running before we can modify a config of any pin.
//  }
//  ControlField confword(registerAddress(bitnum & 8 ? 4 : 0), (bitnum & 7) << 2, 4);// &7:modulo 8, number of conf blocks in a 32 bit word.; 4 bits each block
//  confword= code;
//}

#pragma clang diagnostic pop
