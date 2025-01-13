/**
// Created by andyh on 12/4/24.
// Copyright (c) 2024 Andy Heilveil, (github/980f). All rights reserved.
*/

#pragma once

/** The DMA controllers across parts are quite different, but serve the same functionality.
* Most transfers are peripheral <-> memory.
Memory to Memory is for memcpy functionality, but the startup code is such that unless the objects are quite large DMA is not a good choice.

103,407,452 have circular mode, restarts the same buffer.
407,411 have double buffered mode, two memory addresses toggle,
103,452 have fixed device to channel connections, 407,411 rather arbitrary.

#transfers max is 64k-1 in most manuals, expect that one manual got that wrong when it had a full 64k.
 */

#include "buffer.h"

struct DmaTransfer {
  struct Stream {
    void *address; //starting point
    unsigned size; //bytes per transfer
    int increment; //address change per transfer, can be zero, negative for descending address
    Stream(void *address, unsigned size, int increment): address(address), size(size), increment(increment) {};
    //default copiers and movers are perfectly fine.
  } source, target;

  unsigned count; //number of source items, target must adjust for size differences.

  static template<typename Scalar> DmaTransfer receive(Scalar &peripheralAddress, );
};

struct DmaState {
  enum {
    Defined, Requested, Active, CompletedOk, Failed
  } state;

  DmaTransfer *transfer = nullptr;
  //most implementations have chaining, if enough do then we might move the 'next' into the DmaTransfer itself.
  DmaTransfer *next_transfer = nullptr;
};
