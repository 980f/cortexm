#pragma once

#include "charscanner.h"
#include "hook.h"
#include "nvic.h"
#include "uart.h"

class UartUser {
  Uart u;
using Agent = Hook<int>;
  Agent agent;
  ByteScanner xmitter;

public:
  
  UartUser(Agent agent, int uartluno, int altpins = 0) : u(uartluno, altpins), agent(agent) {
    //# do nothing to allow for easy static init.
  }

public:
  int byteErrors;
  int ffErrors;
  /** uart compatible init args:*/
  void init(u32 baud = 115200, char parity = 'E', int bitsper = 8, bool paceInput = false);

  void reception(bool beOn) {
    u.beReceiving(beOn);
  }
  /** @param buffer gets sliced into, you can't sanely modify it until transmission is complete.*/
  bool transmit(ByteScanner &buffer);
  bool transmit(CharScanner &buffer);

  /** @return whether a call to transmit() will succeed.
can only send if last message is completely sent.*/
  bool canTransmit(void) {
    return !xmitter.hasNext();
  }
  /** now can touch hardware:*/
  void setPriority(u8 level);
  /** uart data interrupt*/
  void uisr(void);
};