#include <stm32.h>

#include "flashcontrol.h"
#include "peripheral.h"


//OptionByte * const UserOption = reinterpret_cast <OptionByte *> (0x1FFFF800);

union CR {
  struct {
    unsigned int latency : 3; //wait states, >48MHz:2 >24MHz:1 else 0
    unsigned int halfCycle : 1; //true if main clock is so slow that flash can run at twice its rate.
    unsigned int prefetch : 1; //
    volatile unsigned int prefetching : 1;
  }
  field;
  unsigned int all;
};

struct FlashController {
  CR cr;

  unsigned int mainKey; //key for program memory
  unsigned int optKey; //key for options memory
  //SR
  volatile unsigned int burning : 1;
  unsigned int : 1;
  volatile unsigned int wasntBlankError : 1;
  unsigned int : 1;
  volatile unsigned int wasWriteProtectedError : 1;
  volatile unsigned int opComplete : 1; //write a 1 to clear this.
  unsigned int : 32 - 6;
  unsigned int burnCode : 1;
  unsigned int erasePage : 1;
  unsigned int eraseAllCode : 1;
  unsigned int : 1;
  unsigned int burnOption : 1;
  unsigned int eraseOption : 1;
  unsigned int execute : 1;
  unsigned int lock : 1;
  unsigned int : 1;
  volatile unsigned int optionsAreWritable : 1;
  unsigned int errorIE : 1;
  unsigned int : 1;
  unsigned int opcompleteIE : 1;
  unsigned int : 32 - 13;

  unsigned int address;
  unsigned int : 32;

  unsigned int opbytestuff; //not yet encoded, write up by stm is incomprehensible.
  unsigned int writeProtects; //accessed algorithmically.

};

soliton(FlashController, 0x40022000);

void setFlash4Clockrate(Hertz hz){
  int waits = 0;

  if(hz > 48000000) {
    waits = 2;
  } else if(hz > 24000000) {
    waits = 1;
  }
  //direct assignment syntax doesn't work because this hardware doesn't accept byte operations and compiler optimizes for them:
  CR cr = theFlashController.cr;
  cr.field.latency = waits;
  theFlashController.cr.all = cr.all;
} /* setFlash4Clockrate */

///*
//  * will erase if necessary.
//  * Inverts data so that 0's in it are 'erased' in flash.
//  */
//int burnPage(unsigned int pagenumber, Indexer <u8>&inverseData){
//  //todo:3 HSI must be on, doesn't have to be main osc.. it is used directly by flash module.
//  //todo:3 get page size, is 1k on smaller cpu's, 2k on larger.
//  //todo:3 check if erase is required
//  //todo:3 erase page
//  //todo:3 burn only those bits needing to change
//  inverseData.rewind();
////  while(inverseData.hasNext()){
////
////  }
//  return -1;
//} /* burnPage
//end of file
