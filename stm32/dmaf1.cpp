#include "dma.h"

void DmaChannel::setupStream(StreamDefinition&def){
  dcb.enable = 0;
  dcb.circular = 0;
  dcb.incPeripheralAddress = 0;
  dcb.incMemoryAddress = 1;
  dcb.peripheralItemSize = def.devicesize;
  dcb.memoryItemSize = def.itemsize;
  dcb.priority = 0;
  dcb.outBound = sender;
  dcb.M2M = 0;
  dcb.peripheral = def.device;
  dcb.transferCount = def.numItems;
  dcb.memory = def.buffer;
} /* setupStream */

static const int uartmap[][2] = {
  /*transmission/reception pairs*/
  { 4, 5 }, //u1
  { 7, 6 }, //u2
  { 2, 3 }, //u3
  { 7 + 3, 7 + 5 }, //u4, dma2
  //no dma for uart 5
};

int DmaChannel::forUart(int unumber, bool txelserx){
  return uartmap[unumber - 1][txelserx];
}
