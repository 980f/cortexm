#pragma once  // (C) 2020 Andrew L. Heilveil (github.980f)   (totally replaces inherited file of same name)

#include "uart.h"
#include "dma.h"
#include "block.h"

using Ubuffer = Block<uint8_t>;

struct DmaBufferedUart {
  const Uart &uart;
  const Dma::DmaTriad &txSelector;
  const Dma::DmaTriad &rxSelector;

  //allocate both channels, can choose to not use one if it is needed by some other periph.
  Dma::Stream tx;
  Dma::Stream rx;
public:
  /** shared reference to a Uart accessor object from which we look up dma controller and channel numbers to go with the given stream number using internal tables */
  constexpr DmaBufferedUart(const Uart &uart, unsigned txstream, unsigned rxstream);

public:
  /** receive chunk of expected size */
  void beginRead(const Ubuffer &rxbuff);
  /** send chunk that is ready to go */
  void beginSend(const Ubuffer &txbuff);
};
