#include "dmabuffereduart.h"

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
}
