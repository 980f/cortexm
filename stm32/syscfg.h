//
// Created by andyh on 10/5/20.
//

#ifndef CDCHOST_SYSCFG_H
#define CDCHOST_SYSCFG_H

#include "stm32.h"
#include "gpio.h"

struct SysCfg : public APBdevice {

  const ControlBit IOCompCell;//#wronglyu report as unused as it is write-only
  const ControlBit rmii;//presently unused as HAL inits it and we don't want to init twice until we know that is OK

  SysCfg():APBdevice(APB2,14)
  ,IOCompCell(registerAddress(0x20),0)
  ,rmii(registerAddress(0x04),23)
  {}

  /** each event comes from the related bit of some GPIO port.*/
  void selectEvent(const Pin &pin) const {
    field(8 + pin.bitnum / 4, 4 * (pin.bitnum % 4), 4) = pin.port.slot;//conveniently slot is what we need here.
  }
};

extern const SysCfg SysConfig;
#endif //CDCHOST_SYSCFG_H
