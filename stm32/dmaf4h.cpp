#include "dmaf4h.h"

/**
 * registers: interrupt status if one of their brain dead non-uniform allocation deals. 6 bits, 6bits, skip 4 and so on.
 * offset 0 + 2 (chnum/2) + 6
 * there is an interrupt vector for each channel, but they are not uniformly allocated.
 *
 * so: compute a clear bit,  tabulate an irq number.
 * then you have a 32 bit word of packed controls
 *  counter is number of items, not (necessarily) number of bytes.
 *  addresses, one peripheral, 2 memory for expeditious double buffering.
 *
 *  The stream is the dma agent, the channel is the request source.
 *
 *  For instance, Uart1 TX is only available on DMA2:7 while its rx can be DMA2:5 or DMA2:2.
 *
 *  Stream object can be precreated as const's, if only the compiler would allow for that.
 *
 * */

Dma::Dma(unsigned stluno, unsigned stream){


}
