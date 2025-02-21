#include "serialport.h"

void SerialPort::uisr() {
  Uart::Status sr;
  sr.flags = dcb.SR; //some bits clear on read, can't use bit band

  if (sr.flags & 15) { //bit error or the garbage that transceiver turnon creates
    ++byteErrors;
    //both overruns and framing errors need a read of the DR to reset the error flag:
    dcb.DR; //try to clear an overrun error, manual says so
    onReception(-2);//todo:M symbols
    return;
  }
  if (sr.dataAvailable) {
    int incoming = dcb.DR; //must read else interrupt persists (and we spin)
    if (incoming == 0x1FF) { //bit error or the garbage that transceiver turnon creates
      ++ffErrors;
      //swallowing, perhaps will pass magic code to onReception as we further extract uart module.
    } else {
      //todo:3 strip parity bit depending on settings, until then presume 8 bit data
      onReception(incoming);
    }
  }
  //byprocessing reads before writes the reception routine can generate a char that will immediately get sent. 
  if (b.transmitAvailableIE && sr.transmitBufferEmpty) { //#then must write to DR or clear transmitAvailableIE
    auto c=nextChar();
    if (c!=~0u) {
      b.enableTransmitter = 1;//when did this get lost, and can we set it and forget it earlier than this?
      dcb.DR = c;
    } else {
      b.transmitAvailableIE = 0; //else we spin in the isr
      b.transmitCompleteIE = 1;
    }
  } else if (b.transmitCompleteIE && sr.transmitCompleted) { //#then must write to DR or clear transmitAvailableIE
    b.transmitCompleteIE = 0;
  }
} /* uisr */

void SerialPort::init(const SerialConfiguration &cfg, bool paceInput) {
  Uart::init(cfg.baud, cfg.parity, cfg.bitsper);
  if (paceInput) {//you must also actually have configured the pins for this purpose.
    handshakers(true,true);
  }
  byteErrors = 0;
  ffErrors = 0;
} /* init */
