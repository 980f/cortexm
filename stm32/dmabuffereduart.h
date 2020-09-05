#ifndef dmabuffereduartH
#define dmabuffereduartH

//#include "buffer.h"
#include "eztypes.h"

/**
  * wrap an existing array so that all of its attributes are available for dma
  */
class RawBuffer { //todo:stub class, needs use case analysis!
  u8 *buffer;
  unsigned int allocated;
  unsigned int sizeeach;
  u8 trash;
public:
  RawBuffer(  u8 * buffer, unsigned int length, unsigned int sizeeach){}

  u8 *item(unsigned int index){
    return &buffer[index < allocated ? index : trash];
  }

  unsigned int length(){
    return allocated;
  }

  unsigned int itemSize(){
    return sizeeach;
  }
};
#define RawWrap(objname) RawBuffer((void *) &objname, sizeof(objname), sizeof(typeof(objname)))


#include "uart.h"
#include "dma.h"

class DmaBufferedUart : public Uart {
  //allocate both channels, can choose to not use one if it is needed by some other periph.
  DmaChannel tx;
  DmaChannel rx;
public:
  DmaBufferedUart(int luno,unsigned int alt=0): Uart(luno,alt)
    , tx(DmaChannel::forUart(luno, true), true)
    , rx(DmaChannel::forUart(luno, false), false){
    /* empty braces*/
  }
private:
  void packDmaArgs(DmaChannel::StreamDefinition&def, RawBuffer&rb, int subset );
public:
  void beginRead(RawBuffer&rb, int subset = 0);
  void beginSend(RawBuffer&rb, int subset);
};

#endif /* ifndef dmabuffereduartH */
