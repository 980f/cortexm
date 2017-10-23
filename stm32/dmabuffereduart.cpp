#include "dmabuffereduart.h"

void DmaBufferedUart::packDmaArgs(DmaChannel::StreamDefinition&def, RawBuffer&rb, int subset ){
  def.device = &Uart::dcb->DR;
  def.devicesize = 2; //uart allows for 9 bit words, like 8051 multidrop protocol or forced parity
  def.buffer = rb.item(0);
  def.numItems = subset > 0 ? subset : rb.length();
  def.itemsize = rb.itemSize();
}

void DmaBufferedUart::beginRead(RawBuffer&rb, int subset ){
  rx.beRunning(0);
  rx.clearInterrupts();
  DmaChannel::StreamDefinition def; //see dma.h
  packDmaArgs(def, rb, subset);
  rx.setupStream(def);
  b->dmaReception = 1;
  b->enableReceiver = 1;
  rx.beRunning(1);
} /* beginRead */

void DmaBufferedUart::beginSend(RawBuffer&rb, int subset){
  tx.beRunning(0);
  tx.clearInterrupts();
  DmaChannel::StreamDefinition def; //see dma.h
  packDmaArgs(def, rb, subset);
  tx.setupStream(def);
  b->dmaTransmitter = 1;
  b->enableTransmitter = 1;
  tx.beRunning(1); //should pick up 1st datum immediately.
}
