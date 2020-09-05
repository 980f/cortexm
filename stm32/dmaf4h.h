#ifndef USBHOST_DMAF4H_H
#define USBHOST_DMAF4H_H "(C) 2020 Andy Heilveil (980F)"

/**
 * dma channel access using HAL as much as feasible, but hiding the laborious STM C interface
 * To get separate compilation we have to lazy init their structs, so we need "permalloc".
 * Let us do that late, for now all the gibberish will be available in all modules <sigh>
 * */
#include "stm32f4xx_hal_dma.h"

class Dma {
  DMA_HandleTypeDef hal;
public:
  Dma(unsigned stluno, unsigned channel);
};

#endif //USBHOST_DMAF4H_H
