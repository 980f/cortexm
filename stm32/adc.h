#ifndef ADC_H
#define ADC_H

#include "stm32.h"
#include "shadow.h"

//tag for ADC value type
using AdcValue=uint16_t ;

constexpr unsigned MaxAdcClock=
#if DEVICE==103
  14'000'000;
#elif DEVICE==407
  30'000'000;
#endif

/**
 * The name was changed from ADC to ADCdev due to compilation issues with STM32 Hal using a #define for ADC
 * todo:1 replace all the structures with ControlWord et al. and ditch the structures.
 * If we ever use the "journal" feature we can make an accessor for that which uses ControlWord et al.
 * */
class ADCdev : public APBdevice {
public:

  ADCdev(unsigned luno);
  void init();

  void convertChannel(unsigned channelcode);

  AdcValue readConversion();

  void configureInput(unsigned channel);

  /** default vrefmV value is for the stm32 internal Vref */
  float milliVolts(AdcValue reading, AdcValue vrefReading, float vrefmV = 1200.0);

  /** base clock, not conversion rate. 30MHz max for F407, 14MHz max for F103*/
  unsigned setClock(unsigned hertz=MaxAdcClock);

  class TrefCalibration {
    float Tcal;
    float mvAtTcal;
    float TpermV; //stm manual: 4.3 mV/'C we need .01'C/mV
public:
    TrefCalibration(float Tcal = 19.50, float mvAtTcal = 1406, float TpermV = 0.23256);
    float celsius(float millis);
  }
  dieTemperature;

  //not encoded is common block of F407, which has joint status comprised of the 3 units and controls for synchronized sampling and DMA.
  //it will be at registers(0x300 + 0|4|8) and can probably just have function calls to access it versus structures.
};
#endif // ADC_H
