#pragma once //(C) 2020 Andrew Heilveil (980F)

#include "peripheraltypes.h"

const Address FlashController(0x40023C00);

//flash control functions for F40x parts
inline void setFlash4Clockrate(unsigned int hz){
  ControlField(FlashController,0,3)=hz/30000000;//full voltage. For lesser voltage change the denominator
}

const ControlBit Prefetch(FlashController,8);
const ControlBit IcacheEnable(FlashController,9);
const ControlBit DcacheEnable(FlashController,10);
//todo: reset bits need some protocol, and when would you manipulate them?
