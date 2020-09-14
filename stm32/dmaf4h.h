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


//use the following where a decimal number of the interrupt request is expected.
#define DmaIrq(luno,stream) ResolveIrq(DMA##luno##_Stream##stream##_irq)

#define  DMA1_Stream0_irq 11
#define  DMA1_Stream1_irq 12
#define  DMA1_Stream2_irq  13
#define  DMA1_Stream3_irq  14
#define  DMA1_Stream4_irq  15
#define  DMA1_Stream5_irq  16
#define  DMA1_Stream6_irq  17
#define  DMA1_Stream7_irq 47

#define  DMA2_Stream0_irq 56
#define  DMA2_Stream1_irq  57
#define  DMA2_Stream2_irq  58
#define  DMA2_Stream3_irq  59
#define  DMA2_Stream4_irq  60
#define  DMA2_Stream5_irq 68
#define  DMA2_Stream6_irq  69
#define  DMA2_Stream7_irq  70
// end of pieces needed for DmaIrq macro}


#endif //USBHOST_DMAF4H_H
