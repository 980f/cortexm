#pragma once
#include "eztypes.h"



/**
  * option bytes, 16 bytes of nv that won't get overwritten by code programming
  */
struct OptionByte {
  u8 normal;
  u8 complement;
  bool isValid() const {
    return normal == u8(~complement); //#ignore warning about promoting ~unsigned
  }
};

#if 0 //still migrating to c++11
//array of 16 OptionBytes.
extern OptionByte *const UserOption;
#endif

//needs to decouple from buffer
#if 0
#include "buffer.h"
/** burn a page with the COMPLEMENT of the given data.
  * @return quantity of failures.
  * granularity of flash is 16 bits.
  * Write protect is on 4k boundarys, which is either 4 or 2 pages depending upon flash size.
  * Flash size can be read from system space at an address that is ambiguously documented.
  */
int burnPage(unsigned int pagenumber, Indexer <u16>&inverseData);
#endif

void setFlash4Clockrate(Hertz hz);
