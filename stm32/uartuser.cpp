#include "uartuser.h"

void UartUser::uisr() {
  //register bool retrigger = false;
  USART_DCB::Status sr;

  sr.flags = u.dcb.SR;  //some bits clear on read, can't use bit band
  if (sr.flags & 15) {  //bit error or the garbage that transceiver turnon creates
    ++byteErrors;
    //both overruns and framing errors need a read of the DR to reset the error flag:
    u.dcb.DR;   //try to clear an overrun error, manual says so
    agent(-2);  //todo:M symbols
    return;
  }
  if (sr.dataAvailable) {
    int incoming = u.dcb.DR;  //must read else interrupt persists (and we spin)
    if (incoming == 0x1FF) {  //bit error or the garbage that transceiver turnon creates
      ++ffErrors;
      //swallowing, perhaps will pass magic code to onReception as we further extract uart module.
    } else {
      //todo:3 strip parity bit depending on settings, until then presume 8 bit data
      agent(incoming);
    }
  }
  if (u.b.transmitAvailableIE && sr.transmitBufferEmpty) {  //#then must write to DR or clear transmitAvailableIE
    if (xmitter.hasNext()) {
      u.dcb.DR = xmitter.next();
    } else {
      u.b.transmitAvailableIE = 0;  //else we spin in the isr
      u.b.transmitCompleteIE = 1;
    }
  } else if (u.b.transmitCompleteIE && sr.transmitCompleted) {  //#then must write to DR or clear transmitAvailableIE
    u.b.transmitCompleteIE = 0;
  }
} /* uisr */

void UartUser::init(u32 baud, char parity, int bitsper, bool paceInput) {
  u.init(baud, parity, bitsper);
  if (paceInput) {           //conditional for debug
    u.takePins(0, 0, 1, 1);  //#note: this doesn't untake the unselected ones.
    u.b.RTSEnable = 1;
    u.b.CTSEnable = 1;
  }
  byteErrors = 0;
  ffErrors = 0;
} /* init */

bool UartUser::transmit(CharScanner& buffer) {
  ByteScanner punt(buffer);
  return transmit(punt);
}

bool UartUser::transmit(ByteScanner& buffer) {
  if (!canTransmit()) {
    return false;  //still sending last blob
  }
  xmitter.grab(buffer);
  u.beTransmitting(xmitter.hasNext());
  return true;
} /* transmit */