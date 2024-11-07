#include "clocks.h"

#include "lpcperipheral.h"

#include "wdt.h"
#include "bitbasher.h"
#include "minimath.h" //multiply divide

//LPC common IR frequency
const u32 LPC_IRC_OSC_CLK(12000000);
//family limit
const u32 MaxFrequency(72000000);

enum ClockSource {
  IRCosc=0,
  PLLin,
  WDTosc,
  PLLout
};

using namespace LPC;

unsigned pllOutput(bool forUsb){
  const u32 &controlreg(*sysConReg(forUsb?0x10:8));
  u32 multiplier=extractField(controlreg,4,0);
  u32 divider=extractField(controlreg,6,5);

  return muldivide( pllInputHz(forUsb) , 1 +multiplier , (1<<divider)) ;
}

void setPLL(bool forUsb,unsigned mpy, unsigned exponent){
	unsigned *controlreg(sysConReg(forUsb?0x10:8));
  mergeBits(controlreg,mpy-1,0,5);
  mergeBits(controlreg,exponent,5,2);
}

unsigned pllInputHz(bool forUsb){
  switch(*sysConReg(forUsb?0x48:0x40) & 0x03) {
  case IRCosc:                       /* Internal RC oscillator             */
    return LPC_IRC_OSC_CLK;

  case 1:                       /* System oscillator                  */
    return EXTERNAL_HERTZ;
    // the case 2 was gotten from cmsis code which either is bullshit or undocumented feature of the part
    //  case 2:                       /* WDT Oscillator                     */
    //    return LPC::WDT::osc_hz(theSYSCON.WDTOSCCTRL);
  } // switch
  return 0;
} // pllInputHz

unsigned coreInputHz(ClockSource cksource){
  switch(cksource){
  case IRCosc:
    return LPC_IRC_OSC_CLK;
  case PLLin:
    return pllInputHz(0);
  case WDTosc:
    return WDT::osc_hz();
  case PLLout:
    return pllOutput(0);
  } // switch
  return 0; // hopefully caller pays attention to this bogus value
} // coreInputHz


unsigned coreInputHz(){
  unsigned &mainclksel(*sysConReg(0x70));
  return coreInputHz(ClockSource(mainclksel & 0x03));
}

unsigned coreHz(){
  return rate(coreInputHz(), *sysConReg(0x78));
}

void setMainClockSource(ClockSource cksource){
  //todo:1 check that source is operating!
  if(1){
    cksource=IRCosc;
  }
  *sysConReg(0x70)=cksource;
  //execution halts, pll says it is ready but clearly it is not.
  raiseBit(sysConReg(0x74),0);
  //cmsis code (but not the manual) suggests that we can monitor the above bit to see when the new value has taken effect, but why wait? it is at most something like 2 of the present clocks and one should be changing the main clock well in advance of doing application timing.
}

unsigned clockRate(int which){
  unsigned divreg=0;
  switch (which) {
  case -1://systick
    divreg=0x0B0;
    break;
  case 0: //processor clock
    divreg=0x078;
    break;
    //leaving the following spread out until debugged, 0x90 + (which*4);
  case 1: //ssp0
    divreg=0x094;
    break;
  case 2: //uart
    divreg=0x098;
    break;
  case 3://ssp1
    divreg=0x09C;
    break;
  }
  if(divreg){
    unsigned &divider(*sysConReg(divreg));
    u32 num=coreInputHz();
    return rate(num,divider);
  }
  return 0;
}

int flashWaits(unsigned frequency){
  if(frequency>40000000){
    return 2;
  }
  if(frequency>20000000){
    return 1;
  }
  return 0;
}

void setFlashWait(unsigned frequency, bool before){
  SFRfield<0x4003c010,0,2> fwc;
  int needed=flashWaits(frequency);
  int presently=fwc;
  if(before?(needed>presently):(needed<presently)){
    fwc=needed;
  }
}

bool switchToPll(unsigned mpy, unsigned exponent){
  powerUp(7);// pllPowerdown
  setPLL(0,mpy, exponent);
  //wait for lock, //100uS absolute limit
  for(unsigned trials=120;trials-->0;){
    if(bit(*sysConReg(0x00c),0)){//if locked
      setMainClockSource(ClockSource::PLLout);
      return true;
    }
  }
  return false;
}


void setMainClock(unsigned hz,ClockSource cksource){
  //divider:
  //
}

void warp9(bool internal){
  u32 sourceHz(0);
  if(internal || EXTERNAL_HERTZ==0 ){
    powerUp(0);//irc power on
    sourceHz=LPC_IRC_OSC_CLK;
  } else {
    powerUp(5);//ext osc on, presumes xtal configged, else we should leave it off and test whether an external clock is actually clicking
    sourceHz=EXTERNAL_HERTZ;
  }
  unsigned exponent=3;//max exponent for divisor
  unsigned scaler=muldivide(MaxFrequency,1<<exponent,sourceHz);//maximum multipler

  while(scaler>32 || scaler*sourceHz>=320000000){//field limit, cco limit
    scaler>>=1;
    if(--exponent==0){
      break;
    }
  }
  //todo: if still above 320MHz then system is not reliable.
  while(isEven(scaler) && scaler*sourceHz>=156000000){//reduce freq if we can without going too low
    scaler>>=1;
    if(--exponent==0){
      break;
    }
  }

  if(scaler){//if we have a solution apply it
    setFlashWait(MaxFrequency,true);
    switchToPll(scaler,exponent);
    setFlashWait(MaxFrequency,false);
  }
  //systick has a prescaler in this part. for other vendors it is fixed to the core.
  *sysConReg(0x0B0)=1;//set to 0 if feature is later disabled.
}

void setMCO(ClockSource which, unsigned divider){
  if(divider){
    *sysConReg(0x0E0)=which;
    *sysConReg(0x0E8)=divider;
    raiseBit(sysConReg(0xE4),0);
    //and direct it to a pin
    *atAddress(ioConReg(0x10))=1; //the values for iocon's defy systemization.
  }
}
