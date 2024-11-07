#include "eztypes.h"
#include "nvic.h"
//#include "buffer.h"

/** logical access */
class DmaChannel {
  struct DMA_DCB {
    volatile u32 interruptFlags; //nibble per channel,will access algorithmically.
    u32 interruptClears; //nibble per channel,will access algorithmically.
    struct CHANNEL_DCB {
      unsigned int enable : 1;
      unsigned int transferCompleteIE : 1;
      unsigned int halfCompleteIE : 1;
      unsigned int errorIE : 1;
      unsigned int outBound : 1;
      unsigned int circular : 1; //auto restart on end of block
      unsigned int incPeripheralAddress : 1; //associated with handshakes for pacing
      unsigned int incMemoryAddress : 1;
      unsigned int peripheralItemSize : 2;
      unsigned int memoryItemSize : 2;
      unsigned int priority : 2;
      unsigned int M2M : 1;  //when set transfers don't expect peripheral handshakes.
      unsigned int : 32 - 15;

      unsigned int transferCount; //only 16 bits are active. There is a shadow register that does the actual count, this does NOT change during operation.
      volatile void *peripheral; //expects handshake unless M2M is set.
      void *memory; //never expects handshake
      unsigned int reservedspace; //20 bytes per channel
    };
    CHANNEL_DCB chan[7];  //index with st's number -1
  };
  //order matters for convenient initialization in constructor:
  const bool secondaryController; //which controller, handy for debug of constructor
  const int luno; //channel within controller, 0 based
  DMA_DCB * const theDMA_DCB;
  DMA_DCB::CHANNEL_DCB &dcb; //cached pointer to theDMA_DCB.chan[luno]
  const bool sender; //direction is builtin to each use, a sender is like uart TX, mem ->peripheral
public:
  Irq irq; //todo: justify irq being public.

  DmaChannel(int stNumber, bool forsender): //pass stnumber as 1..7 for dma controller 1, 8..12 for DMA2.
    secondaryController(stNumber > 7),
    luno(stNumber - (secondaryController ? 8 : 1)), //which channel of the controller
    theDMA_DCB(reinterpret_cast <DMA_DCB *> (secondaryController ? 0x40020400 : 0x40020000)), //
    dcb(theDMA_DCB->chan[luno]),
    sender(forsender),
    irq((stNumber == 12) ? 59 : (stNumber + (secondaryController ? 48 : 10))){ //channels 4&5 share an interrupt.
    /* all is in initlist */
  }

  //clear all interrupts
  DmaChannel&clearInterrupts(void){
    theDMA_DCB->interruptClears |= 0xF << (luno * 4);
    return *this;
  }

  DmaChannel&beRunning(bool on){
    dcb.enable = on;
    return *this;
  }

  DmaChannel&tcInterrupt(bool on){
    theDMA_DCB->chan[luno].transferCompleteIE = on;
    return *this;
  }

  DmaChannel&hcInterrupt(bool on){
    theDMA_DCB->chan[luno].halfCompleteIE = on;
    return *this;
  }

  DmaChannel&errorInterrupt(bool on){
    theDMA_DCB->chan[luno].errorIE = on;
    return *this;
  }

//for what would otherwise be an annoying argument list to a function.
  struct StreamDefinition {
    volatile void *device; //registers that are read are marked volatile.
    int devicesize;
    void *buffer;
    int numItems;
    int itemsize;
  };

  void setupStream(const StreamDefinition& def);

  static int forUart(int unumber, bool txelserx);
};
/**
  * NOTE: mistmatched source and destination sizes does not result in 1:N moves, data is truncated or zero padded.
  * A few destinations may duplicate the lower data in the upper parts when expansion is configured.
  */
