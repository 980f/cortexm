#include "gpio.h"
#include "peripheralband.h" //deprecated bandFor
#include "bitbanger.h"
// priority must be such that these get created before any application objects.

const Port PA InitStep(InitHardware) ('A');
const Port PB InitStep(InitHardware) ('B');
const Port PC InitStep(InitHardware) ('C');
const Port PD InitStep(InitHardware) ('D');
const Port PE InitStep(InitHardware) ('E');
// todo:3 use device define to add ports up to G.

Port::Field::Field(unsigned pincode, const Port &port, unsigned lsb, unsigned msb):
  odr(port.odr()),
  at(port.registerAddress(0x10)),
  lsb(lsb),
  mask(fieldMask(msb, lsb) | fieldMask(msb, lsb) << 16){
  // and actually set the pins to their types
  for(unsigned abit = lsb; abit <= msb; ++abit) {
    port.configure(abit, pincode);
  }
}

void Port::Field::operator =(unsigned value) const {
  ControlWord field(at);
  field = mask & ((unsigned(~value << 16) | value) << lsb); // read the stm32 manual for this.
}


Port::Field::operator u16(void) const {
  return (odr & mask) >> lsb;
}

void Port::Field::operator ^=(unsigned value) const {
  return *this = (value ^ *this); // uses operator = and operator cast u16.
}

u16 Port::Field::actual() const {
  u16 actually = (&odr)[-2]; // idr precedes odr, -2 is for 2 u16's.

  return (actually & mask) >> lsb;
}

///////////////////////////

void Pin::configureAs(unsigned int code) const {
  port.configure(bitnum, code);
}

void Pin::output(unsigned int code, unsigned int mhz, bool openDrain) const {
  code |= openDrain << 2;
  switch(mhz) {
  default: // on any errors be a slow output
  case 2: code |= 2; break;
  case 10: code |= 1; break;
  case 50: code |= 3; break;
  }
  configureAs(code);
}

Pin::Pin(const Port &port, unsigned bitnum): bitnum(bitnum), port(port){/*empty*/}

const Pin &Pin::AI() const {
  configureAs(0);
  return *this;
}

ControlWord Pin::DI(char UDF) const { // default Down as that is what meters will do.
  writer() = bit(UDF, 0); // ODR determines whether a pullup else a pulldown is connected ... this takes advantage of the ascii codes for U and D differing in the lsb.
  // NB: if the pin is already an output then the above line pulses current into it a moment before the next line turns off the driver. This is typically a good thing.
  configureAs((UDF == 'F') ? 4 : 8); // ... this determines if the pin actually gets pulled.
  return ControlWord(reader());
}

ControlWord Pin::highDriver() const {
  Address confword=port.registerAddress((bitnum >= 8) ? 4 : 0);
  unsigned bitoff = (bitnum & 7) * 4 + 2;

  return ControlWord(confword, bitoff);
}

ControlWord Pin::DO(unsigned int mhz, bool openDrain) const { // volatile to prevent removal by optimizer
  output(0, mhz, openDrain);
  return ControlWord(writer());
}

const Pin &Pin::FN(unsigned int mhz, bool openDrain) const {
  output(8, mhz, openDrain);
  return *this;
}

ControlWord Pin::reader() const {
  return ControlWord(port.registerAddress(8), bitnum);
}

ControlWord Pin::writer() const { // volatile to prevent removal by optimizer
  return ControlWord(port.registerAddress(12), bitnum);
}

//////////////////////////////////

InputPin::InputPin(const Pin &pin, char UDF, bool lowactive): LogicalPin(pin.DI(UDF),lowactive){
  /*empty*/
}

InputPin::InputPin(const Pin &pin, bool lowactive): InputPin(pin,lowactive, lowactive ? 'U' : 'D'){
  /*empty*/
}

//////////////////////////////////

OutputPin::OutputPin(const Pin &pin, bool lowactive, unsigned int mhz, bool openDrain):
  LogicalPin(pin.DO(mhz, openDrain),lowactive){
  /*empty*/
}

void OutputPin::toggle(){
  bitbanger=1-bitbanger;//we can ignore polarity stuff :)
}

/////////////////////////////////

bool Port::isOutput(unsigned pincode){
  return (pincode&3)!=0;//if so then code is Alt/Open
}

Port::Port(char letter): APBdevice(2, 2 + unsigned(letter - 'A')){}

void Port::configure(unsigned bitnum, unsigned code) const {
  if(! isEnabled()) { // deferred init, so we don't have to sequence init routines, and so we can statically create objects wihtout wasting power if they aren't needed.
    init(); // must have the whole port running before we can modify a config of any pin.
  }
  ControlField confword(registerAddress((bitnum & 8) ? 4 : 0),(bitnum&7)<<2,4);// &7:modulo 8, number of conf blocks in a 32 bit word.; 4 bits each block
  confword= code;
}



LogicalPin::LogicalPin(Address registerAddress, bool active):
  bitbanger(registerAddress),
  active(active){
  /*empty*/
}
