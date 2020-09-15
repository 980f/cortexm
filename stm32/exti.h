#pragma once

#include "stm32.h"
/**
stm32F's wildly annoying interrupt input manager

inputs 0:4 have individual interrupts, 6+#.
inputs 5..9 share one at 23,
inputs 10..15 share another at 40.

If bit n of one port is watched then no other port's bit n can be watched.

refs gpio port so that enabling an interrupt reconfigures the pin to be an input */

#include "gpio.h"
#include "nvic.h" //return irq reference

class Exti :public APBdevice {
  static const Irq irqsome[];
public: //
  static unsigned irqIndex(unsigned pinnumber);

public:
  Exti();

  /** @returns a reference to the shared interrupt control object for the pin.
you can set both rising and falling, then check the level directly to figure out which happened
*/
  static const Irq &enablePin(const Pin &pin, bool rising, bool falling);
  static void clearPending(const Pin &pin);
};

