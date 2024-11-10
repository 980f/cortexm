#include "spi.h"
#include "minimath.h"
/** blocking send*/
void Spi::send8(int lsbyte, bool blockTilDone){
  while(!band->canSend) {}
  band->wideWord = 0;
  dcb->data = lsbyte;
  if(blockTilDone) {
    block();
  }
}
/** blocking send*/
void Spi::send16(int lsword, bool blockTilDone){
  while(!band->canSend) {}
  band->wideWord = 1;
  dcb->data = lsword;
  if(blockTilDone) {
    block();
  }
}

/** blocking send*/
u8 Spi::send(int lspart, bool blockTilDone){
  while(!band->canSend) {}
  dcb->data = lspart;
  if(blockTilDone) {
    block();
  }
  return u8(dcb->data);
}

//0x1FFF F7E8
Spi:: Spi(unsigned zluno): apb(zluno ? 1: 2, zluno ? (zluno + 13): 12), irq((zluno < 2) ? (35 + zluno): 51){
  dcb = reinterpret_cast <DCB *> (apb.blockAddress);
  band = reinterpret_cast <volatile Band *> (apb.bandAddress);
}

/** clock and other pins get configured here for function and speed*/
void Spi::connect(Pin *sck, Pin *mosi, Pin *miso, Pin *ss){
  if(sck) {
    sck->FN(10);
  }
  if(mosi) {
    mosi->FN(10);
  }
  if(ss) {
    ss->FN(10);
  }
  if(miso) {
    miso->DI('D');
  }
} /* connect */


/*set clock rate and frame format*/
void Spi::configure(bool master, unsigned baud, bool sixteen /* future: options for bidirectional etc.*/){
  apb.init(); //see manual for values set by this init().
  dcb->baudDiv = log2Exponent(rate(apb.getClockRate(), 2 * baud)) - 1; //hardware itself does a /2 on top of this divisor ,to support bidi mode.
  band->clockIdleHigh = 0; //todo:3 add param
  band->clockLate = 1; //todo: 3 add param
  band->ssm = 1; //1:no hardware driven NSS pin.
  band->ssi = 1; //internal value used when ssm==1, must be a 1 to choke off slave.
  band->master = master;
  band->wideWord = sixteen;
  band->enable = 1; //?seems to trigger a useless byte cycle.
} /* configure */
//end of file
