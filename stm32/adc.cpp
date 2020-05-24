#include "adc.h"
//#include "buffer.h"
#include "minimath.h"
#include "gpio.h"

//this constructor only supports ADC1 and 2
ADC::ADC(int luno) :
  APBdevice(2, 8 + luno),
  dcb(*reinterpret_cast<ADC_DCB *>(blockAddress)), //
  band(*reinterpret_cast<ADC_Band *>(bandAddress)) {
}

void ADC::init(void) {
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
  //perform calibration
  band.beCalibrating = 1;
  while (band.beCalibrating) { //maximum around 7 uSec.
  }
} /* init */

void ADC::convertChannel(int channelcode) {
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
void ADC::configureInput(unsigned int channel) {
  if (channel < 8) {
    Pin(PA, channel).AI();
  } else if (channel < 10) {
    Pin(PB, channel - 8).AI();
  } else if (channel < 16) {
    Pin(PC, channel - 10).AI();
  } else if (channel < 18) {
    band.enableRefandTemp = 1;
  }
} /* configureInput */

float ADC::milliVolts(u16 reading, u16 vrefReading, float vrefmV) {
  return vrefmV * ratio(float(reading), float(vrefReading));
}

ADC::TrefCalibration::TrefCalibration(float Tcal, float mvAtTcal, float TpermV) :
  Tcal(Tcal), mvAtTcal(mvAtTcal), TpermV(TpermV) {
}

float ADC::TrefCalibration::celsius(float millis) {
  return Tcal - TpermV * (millis - mvAtTcal); //negative tempco.
}

//end of file
