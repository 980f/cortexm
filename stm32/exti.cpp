#include "exti.h"

#if DEVICE==103
#include "afio.h" //to get to selectors for lower 16 exti's
#include "bitbasher.h"
static const Exti theExti InitStep(InitHardware+10);//after ports

void selectEvent(const Pin &pin){
  theAfioManager.selectEvent(pin);
}
#elif DEVICE==407
#include "gpiof4.h"
//syscfg does what afio does for the 103
#include "syscfg.h"
static const Exti theExti InitStep(InitHardware + 10); //after ports

#endif


const Irq Exti::irqsome[5 + 2] = { //psuedo random association of irq with port.
  Irq(6 + 0), Irq(6 + 1), Irq(6 + 2), Irq(6 + 3), Irq(6 + 4), //
  Irq(23), Irq(40), //
};

unsigned Exti::irqIndex(unsigned pinnumber) {
  switch (pinnumber) {
  case 10 ... 15:  //qt colorizer misidentifies this as an error
    return 6;
  case 5 ... 9:    //... it doesn't know about case ranges (yet)
    return 5;
  default:
    return pinnumber;
  }
}

Exti::Exti() :
#if DEVICE==103
APBdevice(2, 1)
#elif DEVICE==407
APBdevice(APB2, 16)
#endif
{
}

const Irq &Exti::enablePin(const Pin &pin, bool rising, bool falling) {
  //call  pin.DI() before calling this method.
#if DEVICE==407
  SysConfig.
#endif
  selectEvent(pin);

  theExti.bit(0, pin.bitnum) = 1;     //interrupt enable
  theExti.bit(0x4, pin.bitnum) = 1;   //also set as an event, mostly to test the .bit method

  theExti.bit(0x08, pin.bitnum) = rising;
  theExti.bit(0x0C, pin.bitnum) = falling;
  return irqsome[irqIndex(pin.bitnum)];
}

void Exti::clearPending(const Pin &pin) {
  theExti.bit(0x14, pin.bitnum) = 1;
}
void Exti::setPending(const Pin &pin) {
  theExti.bit(0x10, pin.bitnum) = 1;
}

