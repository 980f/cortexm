#pragma once //(C) 2020 Andrew Heilveil (980F)

#include "peripheraltypes.h"

const Address FlashController(0x40023C00);

const SFRbandbit<FlashController,8> Prefetch;
const SFRbandbit<FlashController,9> IcacheEnable;
const SFRbandbit<FlashController,10> DcacheEnable;
//todo:M reset bits need some protocol, and when would you manipulate them?
