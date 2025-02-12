#pragma once

#include "uart.h"

struct SerialConfiguration {
  u32 baud = 115200;
  /** Even None Odd Mark Space */
  char parity = 'N'; //'E" was for MODBUS, we will use CAN instead of MODBUS, reducing our misery by one small step.
  int bitsper = 8;
  /** not yet supporting extra stop length */
  explicit SerialConfiguration(u32 baud=115200, char parity='N', int bitsper=8) : baud(baud), parity(parity), bitsper(bitsper) {}
};

class SerialPort: public Uart {
protected:
   /** overriders: this is called from an ISR. Negative values are error events, 0 or positive are a received datum., incoming glitches produce 0x1FF, which is noted as an error rather than a character, which means that 9 bit operation is not really possible on these chips, where the uart seems to glitch even in quiet systems. */
  virtual void onReception(int charOrError){};
  
  /** overiders; @return ~0 for nothing more to send, else return the next char to be sent. 
   * this gets called from an ISR when the uart xmitter is writable.
   * It will get called when you call beTransmitting(true);
   */
  virtual unsigned nextChar(){return ~0;}

public:
  SerialPort(int uartluno): Uart(uartluno){}
  int byteErrors=0;
  int ffErrors=0;
  /** baud etc, @param paceInput enables RTS/CTS hardware */
  void init(const SerialConfiguration &cfg,  bool paceInput=false);

  /** enable receiver, must independently enable overall interrupt. */
  void reception(bool beOn){
    beReceiving(beOn);
  }

  ///** @param buffer's contents are pointed at, the buffer memory must stay allocated and unmodified.
  // * if getTail is true then from pointer to length is what gets sent, else from 0 to pointer */
  //bool transmit(const Indexer<unsigned char> &buffer, bool getTail=false);
  ///** @see other transmit() this just type casts. */
  //bool transmit(const Indexer<char>&buffer, bool getTail=false);

//  /** @return whether a call to transmit() will succeed.
//can only send if last message is completely sent.*/
//  bool canTransmit(void){
//    return !xmitter.hasNext();
//  }
  /** set interrupt priority */
  void setPriority(u8 level){
    irq.setPriority(level);
  }


  /** uart data interrupt should be routed here. */
  void uisr();

};
