#include "gpio.h"

#ifndef ARDUINO
//#include "minimath.h"

static Handler pinvectors[1+PD][32];//zero-init by cstartup

Handler &gpioVector(unsigned portnum, unsigned pinnumber){
  static Handler byteme;
  if(portnum<3 && pinnumber<32){
    return pinvectors[portnum][pinnumber];
  } else {
    return byteme;
  }
}

void hike(Handler *table,unsigned bits) WEAK; //will provide a handcoded asm version that is twice as fast.
void hike(Handler *table,unsigned bits){
  while(bits){
    unsigned next=log2Exponent(bits);//assembly version is damned fast, however-- should inline it here.
    bits^=1<<next;//flip bit, is actually slightly faster than clearing it
    Handler handler= table[next];
    if(handler!=nullptr){
      (*handler)();
    }
  }
}


#include "nvic.h"

#define GPIOIrqHandler(pidnum,portname) \
HandleInterrupt(pidnum){\
  hike(pinvectors[portname],*atAddress(portBase(portname)+0x4C));\
}

GPIOIrqHandler(11,PA)
GPIOIrqHandler(12,PB)
GPIOIrqHandler(13,PC)
GPIOIrqHandler(14,PD)

#endif
