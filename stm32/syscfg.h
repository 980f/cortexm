//
// Created by andyh on 10/5/20.
//

#ifndef CDCHOST_SYSCFG_H
#define CDCHOST_SYSCFG_H

#include "stm32.h"
#include "gpio.h"

struct SysCfg: public APBdevice {
  SysCfg();
  const ControlBit IOCompCell;
  const ControlBit rmii;


  void selectEvent(const Pin &pin)const {
//8+ num/4, 4 bits wide
    ControlField(registerAddress(8+pin.bitnum/4),4)=pin.port.slot;//conveniently slot is what we need here.
  }
};

extern const SysCfg SysConfig;
#endif //CDCHOST_SYSCFG_H
