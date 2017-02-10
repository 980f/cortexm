/* dro project
 * first user of lpcbase so building it in that directory, will later split into project and library.
 */

#include "lpc13xx.h"
#include "gpio.h" // lpcexpresso
#include "nvic.h" // alh
#include "core_cmInstr.h" // cm3 intrinsics for WFE
#include "cheapTricks.h"
//#include "functional"
#include "systick.h" //periodic behavior
//#include "P1343devkit.h" //for led's

using namespace LPC;

//should inherit from a board description header
const unsigned ExpectedClock=12000000;
// p0-4,p05 for qei.

const InputPin<PortNumber(0), BitNumber(4)> primePhase(BusLatch);
const InputPin<0, 5> otherPhase(BusLatch);

Irq qeiPrimeIrq(4);

int axis;
// prime phase interrupt
HandleInterrupt(4){ // interrupt number and bit number coincidentally the same. Need to add mechanism for this.
  bool dirbit = otherPhase;

  if(dirbit) {
    ++axis; // ignoring quarter phase for now
  } else {
    --axis; // will be +/-4 here
  }
}

// p2-11, p2-10 for index and eot.
/**configuration:
 * polarities for each physical input
 * reverse direction
 */


//#include "limitedpointer.h"
//class BufferedSender {
//  LimitedPointer <const char>pointer;
//public:
//  void setMessage(const char *message,int sizeofMessage){
//    pointer=LimitedPointer <const char>(message,sizeofMessage);
//  }

//  /** called by isr when transmission becomes possible.
//  *  @return either an 8 bit unsigned character, or -1 to disable transmission events*/
//  int next(){
//    return pointer?pointer.next():-1;
//  }
//};
//BufferedSender UartMessage;

#include "fifo.h"
unsigned char messageBuf[256];
Fifo ogmessage(256,messageBuf);

int nextDebugChar(){
  return ogmessage.remove();//conveniently aligned exceptional return value
}

const char testMessage[]="Jello Whirled\r\n";


int wtf(int complaint){
  static int lastWtf = 0;
  static int repeats = 0;

  if(complaint) {
    if(changed(lastWtf, complaint)) {
      repeats = 0;
    } else {
      ++repeats;
    }
  }
  return complaint;
} // wtf

#include "uart.h"

P1343devkit kit;
CyclicTimer ledToggle;

int main(void){
  //enable debug output:
  theUart.setTransmitter(nextDebugChar);
  theUart.setBaud(115200,ExpectedClock);

  axis = 0;
  // interrupt defaults to edge sensitive
  qeiPrimeIrq.enable();


  startPeriodicTimer(1000);//shoot for millisecond resolution
  ledToggle.restart(0.75);//seconds
  while(1) {
    __WFE();//WFE is more inclusive than WFI, events don't call an isr but do wakeup the core.
    if(ledToggle){
      kit.led3=!kit.led3;
    }
  }
  return 0;//avert useless warning
} // main

#ifdef HOST_SIM // cross compilation fakeouts
//// vendore specific code must provide a clock source, architecture varies greatly across vendors.
//unsigned clockRate(unsigned which){
//  return 12000000;
//}
#else //target build

#endif
