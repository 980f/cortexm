/** the implementation is partially migrated to using inline templated stuff vs. carefully crafter struct stuff.*/

#include "uart.h"
#include "lpcperipheral.h" // the peripheral is wholly hidden within this module.
#include "gpio.h" // to gain control of pins
#include "clocks1343.h"
#include "minimath.h" // checked divide
#include "bitbanger.h" // for BitField
#include "nvic.h"  // for isr

const Irq<uartIrq> uirq;

using namespace LPC;

// going through one level of computation in expectation that we will meet a part with more than one uart:
constexpr unsigned uartRegister(unsigned offset){
  return apb0Device(2) + offset;
}

constexpr unsigned &uartClockDivider(){
  return *sysConReg(0x98);
}

const SFR8<uartRegister(0)> dataByte;

//interrupt enable register:
constexpr unsigned IER=uartRegister(0x04);
SFRbit<IER,0> receiveDataInterruptEnable;
SFRbit<IER,1> transmitHoldingRegisterEmptyInterruptEnable;
SFRbit<IER,2> lineStatusInterruptEnable;
SFRbit<IER,8> AutoBaudCompleteInterruptEnable;
SFRbit<IER,9> AutoBaudTimeoutInterruptEnable;

//interrupt status register:
constexpr unsigned IIR=uartRegister(0x08);
const SFRbit<IIR,0> NonePending;
const SFRfield<IIR,1,3> InterruptID;
const SFRfield<IIR,6,2> FifoLevelSetting; //see FCR comments.
const SFRbit<IIR,8> AutoBaudCompleteInterrupt;
const SFRbit<IIR,9> AutoBaudTimeoutInterrupt;
//SFR8<uartRegister(0x08)> IIR;//need to read once, then scan bits?


/** receive fifo interrupt level is 1,4,8, or 14:
1: 0001->0
4: 0100->1
8: 1000->2
14:1110->3
so use 2 msbs of given value, illegal value->legal value that is less than the illegal one
*/
//can't do this as register is write-only (read is IIR) ... SFRfield<FCR,6,2> receiveFifoLevel;
const SFR8<uartRegister(0x08)> FCR;

void Uart::setRxLevel(unsigned one48or14) const{
  FCR = 1 | (one48or14>>2);//must have the lsb a 1 else we kill the uart. see manual 12.6.6 table 201.
}


// line control register:
constexpr unsigned LCR = uartRegister(0x0c);
/** number of bit, minus 5*/
SFRfield<LCR, 0, 2> numbitsSub5;
SFRbit<LCR, 2> longStop;
/** only 5 relevant values, only 3 common ones*/
SFRfield<LCR, 3, 3> parity;
/** sends break for as long as this is true */
SFRbit<LCR, 6> sendBreak;
/** the heinous divisor latch access bit. */
SFRbit<LCR, 7> dlab;

//modem control register at 0x10
SFRbit<uartRegister(0x10),4> loopback;
void Uart::setLoopback(bool on)const{
  loopback=on;
}
/** line status register. reading does not modify it, to clear a bit you must react appropriately to it.
 * We don't declare bits for it as it needs to be buffered to stay in sync with related data, we'll pick bits from the buffered value.
*/
const SFR8<uartRegister(0x14)> LSR;

/** iopin pattern for uart pins: */
constexpr PinBias pickUart = PinBias(0b11010001); // rtfm, not worth making syntax

void Uart::initializeInternals() const{
  uirq.disable();
  disableClock(UART);
  // the 134x parts are picky about order here, the clock must be OFF when configuring the pins.
  InputPin<PortNumber(1), BitNumber(6)> rxd(pickUart);
  OutputPin<PortNumber(1), BitNumber(7)> txd(pickUart);
  // the 134x parts are picky about order here, the clock must be OFF when configuring the pins.

  /* Enable UART clock */
  enableClock(UART); //
  //system prescaler, before the uart's own 'DLAB' is applied.
  uartClockDivider() = 1U; // a functioning value, that allows for the greatest precision, if in range.

  FCR=7;//enable, clear fifos, minimal fifo threshold
  uirq.enable();//having reset all the controls we won't get any interrupts until more configuration is done.
}

Uart::Uart(Uart::Receiver receiver, Uart::Sender sender):receive(receiver),send(sender){
  initializeInternals();
}

/** @param which 0:dsr, 1:dcd, 2:ri @param onP3 true: port 3 else port 2 */
void configureModemWire(unsigned which, bool onP3){
  *atAddress(ioConReg(0xb4+(which<<2)))=onP3;
}

unsigned Uart::setBaudPieces(unsigned divider, unsigned mul, unsigned div, unsigned sysFreq) const {
  if(sysFreq == 0) { // then it is a request to use the active value
    sysFreq = clockRate(UART);
  }
  if(mul == 0 || mul > 15 || div > mul) { // invalid, so set to disabling values:
    mul = 1;
    div = 0;
  } else if(div == 0) {
    mul = 1; // for sake of frequency computation
  }
  constexpr unsigned FDR = uartRegister(0x28);
  SFRfield<FDR, 0, 4> fdiv(div);
  SFRfield<FDR, 4, 4> fmul(mul);

  dlab = 1;
  *atAddress(uartRegister(0x4))= divider >> 8;
  *atAddress(uartRegister(0x0))= divider;
  dlab = 0;
  return rate((mul * sysFreq) , ((mul + div) * divider * uartClockDivider() * 16));
} // Uart::setBaud

void Uart::setFraming(const char *coded) const {
  unsigned numbits = *coded++ - '0';

  if(numbits < 5) {
    return;
  }
  if(numbits > 8) {
    return;
  }
  numbitsSub5 = numbits - 5;
  switch(*coded++) {
  default:
  case 0: return;

  case 'N':
    parity = 0;
    break;
  case 'O':
    parity = 0b001;
    break;
  case 'E':
    parity = 0b011;
    break;
  case 'M':
    parity = 0b101;
    break;
  case 'S':
    parity = 0b111;
    break;
  } // switch
  int stopbits = *coded++ - '0';
  if(stopbits < 1) {
    return;
  }
  if(stopbits > 2) {
    return;
  }
  longStop = stopbits != 1;
} // Uart::setFraming

void Uart::beTransmitting(bool enabled)const{
  if(enabled){
    if(!transmitHoldingRegisterEmptyInterruptEnable){//if not enabled then send first byte
      int nextch = send();
      if(nextch >= 0) {
        *atAddress(uartRegister(0)) = nextch;
        transmitHoldingRegisterEmptyInterruptEnable=1;
      }
    }
  } else {
    transmitHoldingRegisterEmptyInterruptEnable=0;
    //but leave xmitter sending? for xoff we don't want to lose any data that has been queued.
  }
}

void Uart::reception(bool enabled)const{
  receiveDataInterruptEnable=enabled;  //triggered one with a null byte read
  //how bout line status interrupts? .. yeah add those:
  lineStatusInterruptEnable=enabled;
  //note: not our responsiblity to enable in the NVIC, that normally should be left alone during operation.
}

Uart &Uart::setTransmitter(Uart::Sender sender){
  this->send = sender;
  return *this;
}

Uart &Uart::setReceiver(Uart::Receiver receiver){
  this->receive = receiver;
  return *this;
}

void Uart::irq(bool enabled)const{
  if(enabled){
    uirq.prepare();//clear and enable
  } else {
    uirq.disable();
  }
}

//inner loop of sucking down the read fifo.
void Uart::tryInput() const{
  unsigned LSRValue=LSR;//read lsr before reading data to keep in sync

  do {//execute once even if no data is in fifo, to get line status error to user
    LSRValue &= ~bitMask(5,2);//erase transmit status bits
    int packem=(~LSRValue)<<8;
    if(LSRValue&1){//data is readable
      packem|=dataByte;
    }
    unsigned actioncode=receive(packem);
    switch (actioncode) {
      case ~0://quit receiving
        reception(false);
        break;
      case 0:
        break;
    }
    LSRValue=LSR;
  } while(LSRValue&1);//while something in fifo

}

void Uart::stuffsome() const {
  while(bit(LSR,5)){
    int nextch = send();
    if(nextch < 0) {//negative for 'no more data'
      // stop xmit interrupts if tx fifo empty.
      transmitHoldingRegisterEmptyInterruptEnable=0;
    } else {
      dataByte = nextch;
    }
  }
}

void Uart::isr()const{
  if(!NonePending){
  switch(InterruptID) {
  case 0: // modem
    break; // no formal reaction to modem line change.
  case 1:  // thre
    stuffsome();
    break;
  case 2: // rda
    tryInput();
    break;
  case 3: // line error
    tryInput();
    break;
  case 4: // reserved
    break;
  case 5: // reserved
    break;
  case 6: // char timeout (dribble in fifo)
    tryInput();
    break;
  case 7: //reserved
    break;
  } // switch
  }
  //todo: autobaud interrupts are seperate from ID encoded ones (even though there are enough spare codes for them, sigh).


} // Uart::isr

