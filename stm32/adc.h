#ifndef ADC_H
#define ADC_H

#include "stm32.h"
#include "shadow.h"

struct ADC_CR1 {
  unsigned int watchChannel : 5;
  unsigned int sequenceCompleteIE : 1;
  unsigned int watchdogIE : 1;
  unsigned int injectionIE : 1;
  unsigned int scan : 1;
  unsigned int watchOneChannel : 1;
  unsigned int autoInject : 1;
  unsigned int discontinuousNormal : 1;
  unsigned int discontinuousInjection : 1;
  unsigned int discontinuousLength : 3;
  unsigned int dualMode : 4;
  unsigned int : 2;
  unsigned int watchInjected : 1;
  unsigned int watchRegular : 1;
  //8 unused bits
  ADC_CR1(void){
    pun(u32, *this) = 0;
  }
};

struct ADC_CR2 {
  unsigned int powerUp : 1; //or wakeup
  unsigned int loopForever : 1;
  unsigned int startCalibration : 1;
  unsigned int resetCalibration : 1;
  unsigned int : 4;
  unsigned int dmaEnabled : 1;
  unsigned int : 2;
  unsigned int alignLeft : 1; //true= data dropped in msbs.
  unsigned int injectionTrigger : 3;
  unsigned int enableHardwareInjection : 1;
  unsigned int : 1;
  unsigned int sequenceTrigger : 3;
  unsigned int enableHardwareTrigger : 1;
  unsigned int startInjection : 1;
  unsigned int startSequence : 1;
  unsigned int enableRefandTemp : 1;
  ADC_CR2(void){
    pun(u32, *this) = 0;
  }
};

struct ADC_DCB {
  volatile unsigned int status;
  Shadowed<ADC_CR1,unsigned int> cr1;
  Shadowed<ADC_CR2,unsigned int> cr2;
  unsigned int smp1; //will use algorithmic access or external constant generator.
  unsigned int smp2; //...
  unsigned int joffset[4];
  unsigned int watchHigh;
  unsigned int watchLow;
  unsigned int seq1; //will use algorithmic access or external constant generator.
  unsigned int seq2; //...
  unsigned int seq3; //...
  unsigned int jseq; //will use algorithmic access or external constant generator.
  volatile unsigned int jdata[4];
  volatile int data;

};

struct ADC_Band {
  volatile unsigned int watchDogFired;
  volatile unsigned int sequenceComplete; //sequence complete!
  volatile unsigned int injectionComplete;
  volatile unsigned int injectionStarted; //set by HW, cleared by you.
  volatile unsigned int sequenceStarted;
  volatile unsigned int srwaste[32 - 5];
  unsigned int watchChannel[5];
  unsigned int sequenceCompleteIE;
  unsigned int watchdogIE;
  unsigned int injectionIE;
  unsigned int scan;
  unsigned int watchOneChannel;
  unsigned int autoInject;
  unsigned int discontinuousNormal;
  unsigned int discontinuousInjection;
  unsigned int discontinuousLength[3];
  unsigned int dualMode[4];
  unsigned int cr1waste1[2];
  unsigned int watchInjected;
  unsigned int watchRegular;
  unsigned int cr1waste2[8];

  unsigned int powerUp; //ADON or wakeup
  unsigned int loopForever; //CONT
  volatile unsigned int beCalibrating; //CAL: is a status as well as a control
  unsigned int resetCalibration; //RSTCAL
  unsigned int cr2waste4[4];
  unsigned int dmaEnabled; //DMA
  unsigned int cr2waste2[2];
  unsigned int alignLeft; //ALIGN:: 1: data in 12 msbs.
  unsigned int injectionTrigger[3];
  unsigned int enableHardwareInjection; //JEXTTRIG
  unsigned int cr2waste1[1];
  unsigned int sequenceTrigger[3];
  unsigned int enableHardwareTrigger; //EXTTRIG
  unsigned int startInjection; //SWSTARTJ
  unsigned int startSequence; //SWSTART: starts sequencer.
  unsigned int enableRefandTemp; //TSVREFE
};

class ADC : public APBdevice {
public:
  ADC_DCB &dcb;
  ADC_Band &band;

  ADC(int luno);
  void init(void);

  void convertChannel(int channelcode);

  inline u16 readConversion(void){
    return dcb.data;
  }

  void configureInput(unsigned int channel);

  /** default vrefmV value is for the stm32 internal Vref*/
  float milliVolts(u16 reading, u16 vrefReading, float vrefmV = 1200.0);

  class TrefCalibration {
    float Tcal;
    float mvAtTcal;
    float TpermV; //stm manual: 4.3 mV/'C we need .01'C/mV
public:
    TrefCalibration(float Tcal = 19.50, float mvAtTcal = 1406, float TpermV = 0.23256);
    float celsius(float millis);
  }
  dieTemperature;

};
#endif // ADC_H
