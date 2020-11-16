/*
 * */

#include "adc.h"

#include "minimath.h"
#include "gpio.h"
#include "clocks.h"

enum KnownRegisters {
  cr1 = 0x4
  , cr2 = 0x08
};

/** 3 ADC's are in same peripheral block separate by 256 bytes.*/
constexpr unsigned lunoOffset(unsigned luno) {
  return (luno - 1) * 0x100;
}

//this constructor only supports ADC1 and 2 of the F10x
ADCdev::ADCdev(unsigned luno) :
#if DEVICE == 103
  APBdevice(APB2, 8 + luno)
#elif DEVICE == 407
  APBdevice(APB2, 8) //one slot for all 3
#else
// compilation error will ensue
#endif
{
  //#nada
}

void ADCdev::init(void) {
  APBdevice::init(); //makes registers accessible, following code only need mention things that differ from reset values
  bit(cr2, 0) = 1;
  bit(cr1, 8) = 1; //scan mode enable
  //set all sampling times to the maximum
  field(0x0c, 0, 3 * 9) = ~0;
  field(0x10, 0, 3 * 10) = ~0;

#if DEVICE == 103
  //perform calibration
  band.beCalibrating = 1;
  while (band.beCalibrating) { //maximum around 7 uSec.
  }
#endif
} /* init */

void ADCdev::convertChannel(unsigned channelcode) {
  //conveniently all bits other than the channel code in seq3 can be zero
  field(0x34, 0, 5) = channelcode;
  bit(cr2, 30) = 1; //a trigger
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
  } else {
#if DEVICE == 103
    band.enableRefandTemp = 1;//enables temperature component.
#elif DEVICE == 407
    if (channel == 16) {
      ControlBit(0x4001'2000 /* ADC1 base */ + 0x300, 23) = 1;//enable temperature sensor
    } else if (channel == 17) {
      //built in VDD sensor
    }
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
  return word(0x4c);
}

unsigned ADCdev::setClock(unsigned int hertz) {
  return adcClock(hertz);
}
