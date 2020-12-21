#ifndef spiH
#define spiH

#include "stm32.h"
#include "nvic.h"
#include "gpio.h"

class Spi {
public:
  /** blocking send w/ @returned response*/
  u8 send(int lsbyte, bool blockTilDone);
  /** change wordsize then blocking send*/
  void send8(int lsbyte, bool blockTilDone);
  /** change wordsize then blocking send*/
  void send16(int lsword, bool blockTilDone);
  inline void block(void){
    while(band->busy) {
      //#hardware is not supposed to be able to stay permanently busy when used for output.
    }
  }
private:
  struct DCB {
    unsigned int clockMode : 2;
    unsigned int master : 1;
    unsigned int baudDiv : 3; //value is apb.getClock()>>(this+1)
    unsigned int enable : 1;
    unsigned int lsbFirst : 1;
    unsigned int receptionMode : 3; //ssi,ssm,rxonly
    unsigned int wideWord : 1;
    unsigned int : 1;
    unsigned int crcEnable : 1;
    unsigned int bidi : 2; //0= separate i/o, 2: receive  3: transmit  1:not needed.
    unsigned int : 32 - 16;

    unsigned int dmaRead : 1;
    unsigned int dmaSend : 1;
    unsigned int ssoe : 1;
    unsigned int : 2;
    unsigned int errorIE : 1;
    unsigned int receiveIE : 1;
    unsigned int transmitIE : 1;
    unsigned int : 32 - 8;
    u32 status;
    u32 data; //only 8 or 16 bits are used, see wideWord or _16bits
    u32 polynomial;
    u32 crc_in;
    u32 crc_out;
//remaining content is I2S. will use a different struct to map that usage.
  };

  struct Band {
    u32 clockLate;
    u32 clockIdleHigh;
    u32 master;
    u32 baud[3];
    u32 enable;
    u32 lsbFirst;
    u32 ssi;
    u32 ssm;
    u32 rxonly;
    u32 wideWord;
    u32 insertCrc;
    u32 crcEnable;
    u32 bidiXmit;
    u32 bidirectional;
    u32 skip[32 - 16];

    u32 dmaRead;
    u32 dmaSend;
    u32 ssoe;
    u32 skipp[2];
    u32 errorIE;
    u32 receiveIE;
    u32 transmitIE;
    u32 sskip[32 - 8];
//status word:
    volatile u32 dataAvailable;
    volatile u32 canSend;
    volatile u32 stereoChannel;
    volatile u32 underrun;
    volatile u32 crcError;
    volatile u32 modeFault;
    volatile u32 overrunError;
    volatile u32 busy;
    //remaining content is I2S or words.

  };

  volatile Band *band;
  volatile DCB *dcb;
  APBdevice apb;
public:
  Irq irq;
  Spi(unsigned zluno); //st's number minus one
/** clock and other pins get configured here for function and speed*/
  void connect(Pin *sck, Pin *mosi = 0, Pin *miso = 0, Pin *ss = 0);
  /*set clock rate and frame format*/
  void configure(bool master, unsigned baud, bool sixteen = false /* todo:1 options for bidirectional etc.*/);
};

#endif /* ifndef spiH */
