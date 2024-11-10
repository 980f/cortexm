#include "dmabuffereduart.h"

<<<<<<< HEAD
void DmaBufferedUart::packDmaArgs(DmaChannel::StreamDefinition&def, RawBuffer&rb, int subset ){
  def.device = &Uart::dcb.DR;
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
  b.dmaReception = 1;
  b.enableReceiver = 1;
  rx.beRunning(true);
} /* beginRead */

void DmaBufferedUart::beginSend(RawBuffer&rb, int subset){
  tx.beRunning(false);
  tx.clearInterrupts();
  DmaChannel::StreamDefinition def; //see dma.h
  packDmaArgs(def, rb, subset);
  tx.setupStream(def);
  b.dmaTransmitter = 1;
  b.enableTransmitter = 1;
  tx.beRunning(true); //should pick up 1st datum immediately.
=======
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
using namespace Dma;

constexpr DeviceTriad UartTxTriad[] = {
  {1, {{2, 7}, 4}},
  {2, {{1, 6}, 4}},
  {3, {{1, 3}, 4}},
  {3, {{1, 4}, 7}},
  {4, {{1, 4}, 4}},
  {5, {{1, 7}, 4}},
  {6, {{2, 1}, 5}},
  {6, {{2, 2}, 5}},
  {7, {{1, 1}, 5}},
  {8, {{1, 0}, 5}}
};

constexpr DeviceTriad UartRxTriad[] = {
  {1, {{2, 2}, 4}},
  {1, {{2, 5}, 4}},
  {2, {{1, 5}, 4}},
  {3, {{1, 1}, 4}},
  {4, {{1, 2}, 4}},
  {5, {{1, 0}, 4}},
  {6, {{2, 6}, 5}},
  {6, {{2, 7}, 5}},
  {7, {{1, 3}, 5}},
  {8, {{1, 6}, 5}},
  {0, {{0, 0}, 0}}
};

void DmaBufferedUart::beginSend(const Ubuffer &txbuff) {
  tx.transfer({uart.registerAddress(4)}, {.pointer=txbuff.violated()}, txbuff.quantity(), Dma::Operation::transmit(txSelector.channel, 1), {});
  uart.txDma(true);
}

constexpr DmaBufferedUart::DmaBufferedUart(const Uart &uart, unsigned int txstream, unsigned int rxstream) :
  uart(uart)
  , txSelector(getTriad(UartTxTriad, uart.stluno, txstream))
  , rxSelector(getTriad(UartRxTriad, uart.stluno, rxstream))
  , tx(txSelector)  //  NOLINT(cppcoreguidelines-slicing) implicit downcast to streamId, drops channel member of xxSelector
  , rx(rxSelector) { // NOLINT(cppcoreguidelines-slicing)
//#nada
>>>>>>> 38af48a193bdaa269537e1f7a37b0db25d5a1b03
}
