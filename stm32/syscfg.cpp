//
// Created by andyh on 10/5/20.
//

#include "syscfg.h"

SysCfg::SysCfg() :APBdevice(APB2,14),
  //the ethernet interface makes this useful to keep EMI down:
  IOCompCell(registerAddress(0x20),0)
  ,rmii(registerAddress(0x04),23)
{}

InitStep(InitHardware)
const SysCfg SysConfig;
