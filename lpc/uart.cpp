/** still tuning rom vs ram */

#include "uart.h"
#include "lpcperipheral.h" // the peripheral is wholly hidden within this module.
#include "gpio.h" // to gain control of pins
#include "clocks1343.h"
#include "minimath.h" // checked divide
#include "bitbasher.h" // for BitField
#include "nvic.h"  // for isr

const Irq<uartIrq> uirq;

static unsigned sendings=0;
static unsigned receptions=0;


#include "circularpointer.h"


using namespace LPC;

// going through one level of computation in expectation that we will meet a part with more than one uart:
constexpr unsigned uartRegister(unsigned offset){
  return apb0Device(2) + offset;
}

const SFR16<sysConBase(0x98)> uartClockDivider;

const SFR8<uartRegister(0)> dataByte;

//interrupt enable register:
constexpr unsigned IER=uartRegister(0x04);
const SFRbit<IER,0> receiveDataInterruptEnable;
const SFRbit<IER,1> transmitHoldingRegisterEmptyInterruptEnable;
const SFRbit<IER,2> lineStatusInterruptEnable;
const SFRbit<IER,8> AutoBaudCompleteInterruptEnable;
const SFRbit<IER,9> AutoBaudTimeoutInterruptEnable;

const SFR8<uartRegister(0x08)> IIR;//need to read once, then scan bits? YES, clears some content on read

const SFR8<uartRegister(0x08)> FCR;


/** receive fifo interrupt level is 1,4,8, or 14:
1: 0001->0
4: 0100->1
8: 1000->2
14:1110->3
so use 2 msbs of given value, illegal value->legal value that is less than the illegal one
*/
void Uart::setRxLevel(unsigned one48or14) const{
  FCR = 1 | ((one48or14>>2)<<6);//must have the lsb a 1 else we kill the uart. see manual 12.6.6 table 201.
}


// line control register:
constexpr unsigned LCR = uartRegister(0x0c);
/** number of bit, minus 5*/
const SFRfield<LCR, 0, 2> numbitsSub5;
const SFRbit<LCR, 2> longStop;
/** only 5 relevant values, only 3 common ones*/
const SFRfield<LCR, 3, 3> parity;
/** sends break for as long as this is true */
const SFRbit<LCR, 6> sendBreak;
/** the heinous divisor latch access bit. */
const SFRbit<LCR, 7> dlab;

//modem control register at 0x10
const SFRbit<uartRegister(0x10),4> loopback;

const ClockEnable<UART> myClock;

void Uart::setLoopback(bool on)const{
  loopback=on;
}
/** line status register. reading does not modify it, to clear a bit you must react appropriately to it.
 * We don't declare bits for it as it needs to be buffered to stay in sync with related data, we'll pick bits from the buffered value.
*/
const SFR8<uartRegister(0x14)> LSR;

constexpr u8 uartPattern(){
  return 0b11011001;//digital, something undefined, hysteresis, buslatch, function 1
}

void Uart::initializeInternals() const{
  uirq=0;//.disable();
  // disableClock(UART);
  myClock=0;
  //{ the 134x parts are picky about order here, the clock must be OFF when configuring the pins.
  InputPin<PortNumber(1), BitNumber(6)> rxd;
  rxd.setIocon(uartPattern());//neither uart pin is doa
  OutputPin<PortNumber(1), BitNumber(7)> txd;
  txd.setIocon(uartPattern());//neither uart pin is doa
  //} the 134x parts are picky about order here, the clock must be OFF when configuring the pins.
  /* Enable UART clock */
  //enableClock(UART); //
  myClock=1;
  //system prescaler, before the uart's own 'DLAB' is applied.
  uartClockDivider = 1U; // a functioning value, that allows for the greatest precision, if in range.

  FCR=7;//enable, clear fifos, minimal fifo threshold
  uirq=1;//.enable();//having reset all the controls we won't get any interrupts until more configuration is done.
}

/** @param which 0:dsr, 1:dcd, 2:ri @param onP3 true: port 3 else port 2 */
void configureModemWire(unsigned which, bool onP3){
  *atAddress(ioConReg(0xb4+(which<<2)))=onP3;
}

#if 0 //doc block
@72000000
lpc limit:
/*4500000*/1,15,0,72000000
//ftdi limit:
/*3000000*/1,14,7,72000000

/*115089*/23,10,7,72000000
midi:
/*31250*/144,15,0,72000000
/*19200*/125,8,7,72000000
/*9600*/386,14,3,72000000

@12000000
/*baud*/	divider,	mul,	div, rate
/*115384*/6,12,1,12000000
/*57692*/13,15,0,12000000
/*19181*/23,10,7,12000000
/*31250*/24,15,0,12000000

/*9603*/71,10,1,12000000
#endif

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

  dlab = true;
  *atAddress(uartRegister(0x4))= divider >> 8;
  *atAddress(uartRegister(0x0))= divider;
  dlab = false;
  return rate((mul * sysFreq) , ((mul + div) * divider * uartClockDivider * 16));
}

void Uart::setFraming(unsigned numbits, Uart::Parity parityCode, unsigned stops) const {
  numbitsSub5 = numbits - 5;
  parity = parityCode;
  longStop = stops != 1;
} // Uart::setFraming

bool Uart::beTransmitting(bool enabled)const{
  if(enabled){
    if(!transmitHoldingRegisterEmptyInterruptEnable){//checking for breakpoint
      transmitHoldingRegisterEmptyInterruptEnable=1;//this will immediately be followed by the isr if there is room for a character.
      return true;
    } else {
      //how do we prime the pump?
//      transmitHoldingRegisterEmptyInterruptEnable=0;
//      transmitHoldingRegisterEmptyInterruptEnable=1;    
    }
  } else {
    transmitHoldingRegisterEmptyInterruptEnable=0;
    //but leave xmitter sending? for xoff we don't want to lose any data that has been queued.
  }
  return false;
}

void Uart::reception(bool enabled)const{
  receiveDataInterruptEnable=enabled;  //triggered one with a null byte read
  lineStatusInterruptEnable=enabled;  //often triggers with thre
}


void Uart::irq(bool enabled)const{
  if(enabled){
    uirq.prepare();//clear and enable
  } else {
    uirq.disable();
  }
}
//////////////////////////////////



//inner loop of sucking down the read fifo.
void UartHandler::tryInput(){
  unsigned LSRValue=unsigned(LSR);//read lsr before reading data to keep in sync

  do {//execute once even if no data is in fifo, to get line status error to user
    LSRValue &= ~bitMask(5,2);//erase transmit status bits
    unsigned packem=(~LSRValue)<<8;
    if(LSRValue&1){//data is readable
      packem|=dataByte;
      ++receptions;
    }
    if(!receive(packem)){
    //quit receiving
      reception(false);
      break;
    }
    LSRValue=LSR;
  } while(LSRValue&1);//while something in fifo

}

void UartHandler::stuffsome() {
  while(bit(LSR,5)){
    int nextch = send();
    if(nextch < 0) {//negative for 'no more data'
      // stop xmit interrupts if tx fifo empty.
      transmitHoldingRegisterEmptyInterruptEnable=0;
    } else {
      dataByte = u8(nextch);
      ++sendings;
    }
  }
}

void UartHandler::isr(){
  u8 which=IIR;// must read just once
  if(!bit(which,0)){
    switch(extractField(which,3,1)) {
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

}

bool UartHandler::receive(int incoming) { return false; }

int UartHandler::send() { return -1; }

