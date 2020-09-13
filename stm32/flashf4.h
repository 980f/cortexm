#pragma once //(C) 2020 Andrew Heilveil (980F)

#include "peripheraltypes.h"

const Address FlashController(0x40023C00);
//moved wait-states to clock module, they intersect more than these bits.
////flash control functions for F40x parts
//inline void setFlash4Clockrate(unsigned int hz){
//  ControlField(FlashController,0,3)=hz/30000000;//full voltage. For lesser voltage change the denominator
//}

const SFRbandbit<FlashController,8> Prefetch;
const SFRbandbit<FlashController,9> IcacheEnable;
const SFRbandbit<FlashController,10> DcacheEnable;
//todo: reset bits need some protocol, and when would you manipulate them?
