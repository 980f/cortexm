/*
 * todo: there are many control register differences as to bit location.
 * */

#include "adc.h"

#include "minimath.h"
#include "gpio.h"
#include "clocks.h"

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
  volatile unsigned int overRan;//not present on F103
  volatile unsigned int srwaste[32 - 6];
  //CR1:
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
  unsigned int dualMode[4]; //only F103
  unsigned int cr1waste1[2];
  unsigned int watchInjected;
  unsigned int watchRegular;
#if DEVICE==103
  unsigned int cr1waste2[8];
#elif DEVICE==407
  unsigned int res[2];
  unsigned int overrunIE;
  unsigned int cr1waste2[5];
#endif
  //CR2:
  unsigned int powerUp; //ADON or wakeup
  unsigned int loopForever; //CONT
#if DEVICE==103
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
#elif DEVICE==407
  unsigned int cr2waste6[6];
  unsigned int dmaEnabled; //DMA
  unsigned int DDS;
  unsigned int EOCS;
  unsigned int alignLeft; //ALIGN:: 1: data in 12 msbs.
  unsigned int cr2waste4[4];
  unsigned int injectionTrigger[4];
  unsigned int jonRise;
  unsigned int jonFall;
  unsigned int startInjection; //SWSTARTJ
  unsigned int skip1;
  unsigned int sequenceTrigger[4];
  unsigned int onRise;
  unsigned int onFall;
  unsigned int startSequence; //SWSTART: starts sequencer.
  unsigned int skipanother;
#endif

};

constexpr unsigned lunoOffset(unsigned luno){
  return (luno-1)*0x100;
}
//this constructor only supports ADC1 and 2 of the F10x
ADCdev::ADCdev(unsigned luno) :
#if DEVICE==103
  APBdevice(APB2, 8 + luno),
  dcb(*reinterpret_cast<ADC_DCB *>(blockAddress)), //
  band(*reinterpret_cast<ADC_Band *>(bandAddress))
#elif DEVICE ==407
  APBdevice(APB2, 8 ), //one slot for all 3
  dcb(*reinterpret_cast<ADC_DCB *>(blockAddress+lunoOffset(luno))), //
  band(*reinterpret_cast<ADC_Band *>(bandAddress+bandShift(lunoOffset(luno))))
#else
// compilation error will ensue
#endif
  {
}

void ADCdev::init(void) {
  APBdevice::init(); //makes registers accessible, following code only need mention things that differ from reset values
  dcb.cr2.sequenceTrigger = 7;
  dcb.cr2.dmaEnabled = 0;
  dcb.cr2.enableHardwareTrigger = 1; //MISNOMER, software start is an "external trigger" to the dimwits who spec'd this peripheral.
  dcb.cr2.loopForever = 0; //one round per system timer tick
  dcb.cr2.powerUp = 1; //do this before cal
  dcb.cr2.update(); //apply config settings en masse, field sets not safe with this device due to compiler issues.

  band.scan = 1; //not to be confused with CONT which we call loopForever
  //scan length init's to 0== do one.
  // dcb.seq1=0<<20;//doing single converts

  //set all sampling times to the maximum, 239.5 * 14 is around 20 uS.
  dcb.smp1 = 077777777; //yep, octal as this is packed 3 bit codes, 8 fields
  dcb.smp2 = 07777777777; //yep, octal as this is packed 3 bit codes, 10 fields
#if DEVICE==103
  //perform calibration
  band.beCalibrating = 1;
  while (band.beCalibrating) { //maximum around 7 uSec.
  }
#endif
} /* init */

void ADCdev::convertChannel(unsigned channelcode) {
  //conveniently all bits other than the channel code in seq3 can be zero
  dcb.seq3 = channelcode;
  band.startSequence = 1; //a trigger
}

/** pin 2 adc channel mapping, medium density series:
 * A0..7 ch0..7
 * B0..1 ch8..9
 * C0..5 ch10..15
 * temperature ch16
 * vref17
 */
void ADCdev::configureInput(unsigned channel) {
  if (channel < 8) {
    PA.forAdc(channel);
  } else if (channel < 10) {
    PB.forAdc(channel - 8);
  } else if (channel < 16) {
    PC.forAdc(channel - 10);
  } else if (channel < 18) {
#if DEVICE==103
    band.enableRefandTemp = 1;//enables temperature component.
#endif
  }
} /* configureInput */

float ADCdev::milliVolts(u16 reading, u16 vrefReading, float vrefmV) {
  return vrefmV * ratio(float(reading), float(vrefReading));
}

ADCdev::TrefCalibration::TrefCalibration(float Tcal, float mvAtTcal, float TpermV) :
  Tcal(Tcal), mvAtTcal(mvAtTcal), TpermV(TpermV) {
}

float ADCdev::TrefCalibration::celsius(float millis) {
  return Tcal - TpermV * (millis - mvAtTcal); //negative tempco.
}

//end of file
u16 ADCdev::readConversion() {
  return dcb.data;
}
unsigned ADCdev::setClock(unsigned int hertz) {
  return adcClock(hertz);
}
