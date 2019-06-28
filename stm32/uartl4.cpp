#include "uartl4.h"
#include "minimath.h"
#include "afio.h"
void UartL4::setBaudrate(unsigned int desired){
  //note: the ST manuals are chocked full of nonsense with respect to baud rate setting.
  // just take the input clock rate and divide by the baud rate, round to the nearest integer, and put that value into the 16 bit BRR as if it were a simple integer.
  // you can ignore all the jabber about fractional baudrate and all the dicing and splicing, which under close inspection of ST's code does absolutely nothing.
  unsigned int osc = getClockRate();
  unsigned int newbaud = rate(osc, desired);

  if(newbaud != dcb.BRR) {
    b.enable = 0;
    dcb.BRR = newbaud;
  }
} /* setBaudrate */


u32 UartL4::bitsPerSecond() const {
  u32 divisor = dcb.BRR;
  unsigned int osc = getClockRate();

  return rate(osc, divisor);
}

/**
  * st's manual fails to note that 8 data bits + parity bit means you must set the word length to 9!
  * So: 9 if sending 9 bits and no parity or 8 bits with parity. Set to 8 for 7 bits with parity.
  */
void UartL4::setParams(unsigned int baud, unsigned int numbits, char parityNEO, bool longStop, bool shortStop){ //19200,8,n,1
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
u32 UartL4::bitsPerByte(void) const {
  u32 bits = 1; //the start bit

  if(b.doubleStop) {
    bits += 2;
  }
  if(b.halfStop) { //sorry, not rigged to deal with 1.5 stop bits,
    bits -= 0; //should be decreasing by 0.5 bits.
  }
  if(b._9bits) {
    return bits + 9;
  }
  return bits + 8; //todo:2 7 bits no parity!
} /* bitsPerByte */

/** timer ticks required to move the given number of chars. Involves numbits and baud etc.*/
u32 UartL4::ticksForChars(unsigned charcount) const {
  return charcount * bitsPerByte() * dcb.BRR;
}

void UartL4::beReceiving(bool yes){
  b.dataAvailableIE = yes; //which is innocuous if interrupts aren't enabled and it is cheaper to set it then to test whether it should be set.
  b.enableReceiver = yes;
  b.enable = yes || b.enableTransmitter;
}

void UartL4::beTransmitting(bool yes){ //NB: do not call this when the last character might be on the wire.
  b.transmitCompleteIE = 0;  //so that we only check this on the last character of a packet.
  b.enable = yes || b.enableReceiver;
  b.transmitAvailableIE = yes; //and the isr will send the first char, we don't need to 'prime' DR here.
}

void UartL4::reconfigure(unsigned int baud, unsigned int numbits, char parityNEO, bool longStop, bool shortStop){ //19200,8,n,1
  APBdevice::init();
  setParams(baud, numbits, parityNEO, longStop, shortStop);
}

void UartL4::init(unsigned int baud, char parityNEO, unsigned int numbits){
  reconfigure(baud, numbits, parityNEO);
  takePins(true, true); //after reconfigure as that reset's all usart settings including the ones this fiddles with.
  irq.enable(); //the reconfigure disables all interrupt sources, so enabling interrupts here won't cause any.
}

//uart0 is on bus2, others on bus 1    slot is 14 for uart0  16+luno for others
UartL4::UartL4(unsigned int zluno, unsigned int alt): APBdevice(zluno ? 1: 2, zluno ? (zluno + 16): 14),
  b  (*reinterpret_cast <volatile UartL4Band *> (bandAddress)),
  dcb (*reinterpret_cast <volatile USARTL4_DCB *> (blockAddress)),
  irq((zluno < 3 ? 37: 49) + zluno),
  zluno(zluno),
  altpins(alt){
  //not grabbing pins quite yet as we may be using a spare uart internally as a funky timer.
}

/**
rx's are coded as floating inputs as RX is presumed to always be driven
hsin's are coded as pulled up inputs in case the pin is left unconnected, although really someone
should actively configure the pin's presence and not rely upon our weak pullup.
*/
//got tired of finesse:
#define makeTxPin(P,b) Pin(P, b).FN(rxtxSpeedRange)

static void  grabInput(const Port &PX,int bn, char udf) {
//assignment is to get compiler to not prune the code (although it kindly warned us that it did).
  Pin(PX, bn).DI(udf)=1;
}

#define pinMux(stnum) theAfioManager.remap.uart##stnum=altpins;theAfioManager.remap.update()

void UartL4::takePins(bool tx, bool rx, bool hsout, bool hsin){
  int rxtxSpeedRange=bitsPerSecond()>460e3?10:2;

  switch(zluno) {
  case 0: //st's 1
    pinMux(1);
    if(hsin) {
      grabInput(PA, 11,'U');
    }
    if(hsout) {
      Pin(PA, 12).FN();
    }
    switch(altpins) {
    case 0 :
      if(tx) {
        makeTxPin(PA, 9);
      }
      if(rx) {
        grabInput(PA, 10,'F');
      }
      break;
    case 1 :
      if(tx) {
        makeTxPin(PB, 6);
      }
      if(rx) {
        grabInput(PB, 7,'F');
      }
      break;
    } /* switch */
    break; //todo:M CHECK XRT! this break was missing which caused pins A2,A3 to be taken whenever A9,A10 were.
  case 1: //st's 2
    pinMux(2);
    switch(altpins) {
    case 0:
      if(tx) {
        makeTxPin(PA, 2);
      }
      if(rx) {
        grabInput(PA, 3,'F');
      }
      if(hsin) {
        grabInput(PA, 0,'U');
      }
      if(hsout) {
        Pin(PA, 1).FN();
      }
      break;
    case 1:
      //todo:3 set the AFIO remap field
      if(tx) {
        makeTxPin(PD, 5);
      }
      if(rx) {
        grabInput(PD, 6,'F');
      }
      if(hsin) {
        grabInput(PD, 3,'U');
      }
      if(hsout) {
        Pin(PD, 4).FN();
      }
      break;
    } /* switch */
    break;
  case 2: //uart 3
    pinMux(3);
    //todo:3 add hs lines
    switch(altpins) {
    case 0:
      if(tx) {
        makeTxPin(PB, 10);
      }
      if(rx) {
        grabInput(PB, 11,'F');
      }
      break;
    case 1:
      if(tx) {
        makeTxPin(PC, 10);
      }
      if(rx) {
        grabInput(PC, 11,'F');
      }
      break;
    case 3:
      if(tx) {
        makeTxPin(PD, 8);
      }
      if(rx) {
        grabInput(PD, 9,'F');
      }
      break;
    } /* switch */
    break;
    //todo:3 uarts 4 and 5, which don't suffer from remap options.
  } /* switch */
  b.enableTransmitter = tx; //else the tx pin floats!
  //... would only dynamically play with this for single wire half duplexing as in some SPI modes, better to use a transceiver and
  //... a gpio pin than to play with the chip's own pin.
} /* takePins */
//End of file
