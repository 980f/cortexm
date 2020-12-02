#include "dmaf4h.h"
using namespace Dma;
/**
 *  counter is number of items, not (necessarily) number of bytes.

 *  The stream is the dma agent, the channel is the request source.
 *  Stream object can be precreated as const.
 *
 *  It is unclear as to whether the address and count need to be reset to rerun a transfer.
 *  In circular mode they do not, and doublebuffered mode is a variant of circular.
 *
 *  When pausing a transfer clear the en but then poll for it to actually clear, or perhaps the TCIF is set?
 *  
 *
 * */



//these guys are used for enabling the modules and calculating addresses in Streams
//someone will have to decide to conditionally enable the group if any of its channels are used.
constexpr Device DMA[] = {{1}, {2}};

void Stream::clearFlags() const {
  //todo:0 parallel clear
  fifoError.clearit = true;
  error.clearit = true;
  dmError.clearit = true;
  half.clearit = true;
  done.clearit = true;
}

const Stream &Stream::transfer(AddressCaster target, AddressCaster source, unsigned int quantity, Operation op, Stream::FifoControl fifoing) const {
  stop();
  configure(op, fifoing);
  return transfer(target,source,quantity);
}

void Stream::configure(Dma::Operation op, Stream::FifoControl fifoing) const {
  operation = op;
  fifo = fifoing;
}

const Stream &Stream::transfer(AddressCaster target, AddressCaster source, unsigned int quantity) const {
  stop();
  if (Operation(operation).toPeripheral) {
    peripheralAddress = target.number;
    memoryAddress = source.number;
  } else {
    peripheralAddress = source.number;
    memoryAddress = target.number;
  }
  count = quantity;
  clearFlags();
  return *this;
  return *this;
}

Operation Operation::receive(unsigned int source, unsigned int sizeofperiph, bool packed) {
  Dma::Operation op;//start with defaults, which are not all zero.
  op.psize = sizecode(sizeofperiph);
  // if packed then memsize is same as periph else memsize is 32 bits regardless of periph
  op.msize = packed ? op.psize : sizecode(4);
  op.trigger = source;
  return op;
}

Operation Operation::transmit(unsigned int source, unsigned int sizeofperiph, bool packed) {
  Dma::Operation op = receive(source, sizeofperiph, packed);//happens to be what we need to do here ...
  op.toPeripheral = true;//... then override one defaulted value
  return op;
}
