#include "uart.h"


#include "minimath.h"


void Uart::setBaudrate(unsigned desired) const {
  //note: the ST manuals are chocked full of nonsense with respect to baud rate setting.
  // just take the input clock rate and divide by the baud rate, round to the nearest integer, and put that value into the 16 bit BRR as if it were a simple integer.
  // you can ignore all the jabber about fractional baudrate and all the dicing and splicing, which under close inspection of ST's code does absolutely nothing.
  Hertz osc = getClockRate();
  unsigned newbaud = rate(osc, desired);

  if (newbaud != dcb.BRR) {
    b.enable = 0;
    dcb.BRR = newbaud;
  }
} /* setBaudrate */


unsigned Uart::bitsPerSecond() const {
  unsigned divisor = dcb.BRR;
  auto osc = getClockRate();

  return rate(osc, divisor);
}

/**
  * st's manual fails to note that 8 data bits + parity bit means you must set the word length to 9!
  * So: 9 if sending 9 bits and no parity or 8 bits with parity. Set to 8 for 7 bits with parity.
  */
void Uart::setParams(unsigned baud, unsigned numbits, char parityNEO, bool longStop, bool shortStop) const { //19200,8,n,1
  b.enable = 0;
  //decode char to the control bits:
  b.parityOdd = parityNEO & (1 << 1); //bit 1 is high for Oh versus low for E
  b.parityEnable = parityNEO & 1; ////lsb is 1 for E or Oh, 0 for N.
  b._9bits = numbits == 9 || (numbits == 8 && b.parityEnable);
  b.halfStop = shortStop;
  b.doubleStop = longStop;
  setBaudrate(baud);
}

/** part of char time calculation, includes stop and start and parity, not just payload bits */
unsigned Uart::bitsPerByte() const {
  unsigned bits = 1; //the start bit

  if (b.doubleStop) {
    bits += 2;
  }
  if (b.halfStop) { //sorry, not rigged to deal with 1.5 stop bits,
    bits -= 0; //should be decreasing by 0.5 bits.
  }
  if (b._9bits) {
    return bits + 9;
  }
  return bits + 8; //todo:2 7 bits no parity!
} /* bitsPerByte */

/** timer ticks required to move the given number of chars. Involves numbits and baud etc.*/
unsigned Uart::ticksForChars(unsigned charcount) const {
  return charcount * bitsPerByte() * dcb.BRR;
}

void Uart::beReceiving(bool yes) const {
  b.dataAvailableIE = yes; //which is innocuous if interrupts aren't enabled and it is cheaper to set it then to test whether it should be set.
  b.enableReceiver = yes;
  b.enable = yes || b.enableTransmitter;
}

void Uart::beTransmitting(bool yes) const { //NB: do not call this when the last character might be on the wire.
  b.transmitCompleteIE = 0; //so that we only check this on the last character of a packet.
  b.enable = yes || b.enableReceiver;
  b.transmitAvailableIE = yes; //and the isr will send the first char, we don't need to 'prime' DR here.
}

void Uart::reconfigure(unsigned baud, unsigned numbits, char parityNEO, bool longStop, bool shortStop) const { //19200,8,n,1
  APBdevice::init();
  setParams(baud, numbits, parityNEO, longStop, shortStop);
}

void Uart::init(unsigned baud, unsigned numbits, char parityNEO) const {
  reconfigure(baud, numbits, parityNEO);
  irq.enable(); //the reconfigure disables all interrupt sources, so enabling interrupts here won't cause any.
}

void Uart::handshakers(bool hsout, bool hsin) const {
  b.RTSEnable = hsout;
  b.CTSEnable = hsin;
}

void Uart::txDma(bool beRunning) const {
  //dma is supposed to be ready so ...
  b.dmaTransmitter = beRunning;
  //if we have been doing dma then halt everything, then start again when in interrupt or other mode.
  beTransmitting(beRunning); //and a dma cycle might happen before this call returns.
}

//End of file
