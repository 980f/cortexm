#pragma  once //"(C) 2020 Andy Heilveil (980F)"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "modernize-use-nodiscard"

#include "stm32.h"

#include "utility.h"

using namespace stm32; //not yet sure which parts are in stm32:: vs cortexm::
/* 4002'6000 and 4002'6400 each with 8 streams each stream with 8 trigger sources
 * interrupt flags and clears are groups of 6, 4 in a word, weird boundaries
 * the clear is 32 bits past the flag.
 *
 *
 *
 * */

namespace Dma {

  constexpr Address BaseFor(unsigned luno) {
    return 0x4004'6000 | ((luno >> 1) << 10);//only 1 and 2 are valid operands
  }

  struct Device : public APBdevice {//AHB1 slots 21,22
    constexpr Device(unsigned stluno) : APBdevice(AHB1, 20 + stluno, BaseFor(stluno)) { // NOLINT(google-explicit-constructor)
      //#nada
    }
  };

  /*
RM0090 says you must modify all 32 bits at once, so we will build a struct then write it via type punning.
 The compiler cannot be trusted to emit a proper 'insert field" instruction as it will cycle just a byte, not all 32 bits.
*/
  struct Operation {
    unsigned enable: 1;
    //while the next 4 are in the order of the status and clears we can't tell if we are allowed to use bitband on them
    unsigned dmErrorInterruptEnabled: 1;
    unsigned transferErrorInterruptEnabled: 1;
    unsigned halfWayInterruptEnabled: 1;
    unsigned completeInterruptEnabled: 1;
    unsigned peripheralPaced: 1;
    //instead of a 2 bit field we have a 1 bit field and another bit set by a special call to this struct to preclude setting an invalid combo
    unsigned toPeripheral: 1;
    unsigned mem2mem: 1;
    //forced zero when paced by device! forced 1 when double buffered!
    unsigned circular: 1;
    unsigned incrementPeripheral: 1;
    unsigned incrementMemory: 1;
    unsigned psize: 2;//8:0  16:1 32:2  aka "2 bytes" and "4bytes" exclusive bits
    unsigned msize: 2;
    //if set use 4 rather than psize for "offset size" ie transfer one byte but bump address by 4.
    unsigned pincos: 1;
    //higher value for higher priority
    unsigned priority: 2;
    unsigned doubleBuffer: 1;
    unsigned currentTarget: 1;//which memory address register is active in double buffered mode
    //forced to 0 when in direct mode
    unsigned peripheralBurst: 2;//1,4,8,16
    //forced to 0 when in direct mode
    unsigned memoryBurst: 2;//1:0,4:1,8:2,16:3  number of trailing zeros -1
    unsigned trigger: 3;

    constexpr Operation() :
      enable(false)
      , dmErrorInterruptEnabled(false)
      , transferErrorInterruptEnabled(false)
      , halfWayInterruptEnabled(false)
      , completeInterruptEnabled(false)
      , peripheralPaced(false)
      , toPeripheral(false)
      , mem2mem(false)
      , circular(false)
      , incrementPeripheral(false)
      , incrementMemory(true) //NB
      , psize(sizecode(1))
      , msize(sizecode(4))
      , pincos(false)
      , priority(0)
      , doubleBuffer(false)
      , currentTarget(false)
      , peripheralBurst(0)
      , memoryBurst(0)
      , trigger(0) {
    };

    constexpr Operation(const Operation &other) = default;
    constexpr Operation &operator=(const Operation &other) = default;

    static constexpr unsigned sizecode(unsigned numbytes) {
      return numbytes >> 1;//works for valid input
    }

    /***/
    static constexpr unsigned burstcode(unsigned numbytes) {
      switch (numbytes) {
        case 16:
          return 3;
        case 8:
          return 2;
        case 4:
          return 1;
        default:
          return 0;
      }
    }

    /* locally compute the sanest operation
  * */
    static Operation receive(unsigned source, unsigned sizeofperiph, bool packed = true);

    static Operation transmit(unsigned source, unsigned sizeofperiph, bool packed = true);
  };

  struct StreamId {
    uint8_t luno;
    uint8_t stream;

    constexpr unsigned cword(unsigned offset) const {
      return BaseFor(luno) + offset + 0x18 * stream;
    }
  } PACKED;

  struct DmaTriad : public StreamId {
    uint8_t channel;
  } PACKED;

/** used for making lookup tables of channel for device given dma stream */
  struct DeviceTriad {
    //st's device logicla unit number, 1-based which conveniently lets us cap a table with a zero luno entry.
    uint8_t devluno;
    DmaTriad triad;
  } PACKED;

  constexpr const DmaTriad &getTriad(const DeviceTriad *table, unsigned devluno, unsigned stream) {
    for (; table->devluno; ++table) {
      if (table->devluno == devluno && table->triad.stream == stream) {
        break;
      }
    }
    return table->triad;
  }

  /** enable, indicator, and clear */
  struct InterruptBit {
    ControlBit active;
    ControlBit clearit;
    ControlBit enabled;

  private:
    static constexpr Address offset(unsigned bookbase, StreamId id) {
      return id.cword(bookbase + (id.stream & ~1));
    }

    static constexpr unsigned bit(unsigned stream, unsigned which) {
      return 6 * (stream & 1) + (which ? (which + 1) : 0);
    }

  public:
    constexpr InterruptBit(StreamId id, unsigned which) :
      active(offset(0x00, id), bit(id.stream, which))
      , clearit(offset(0x08, id), bit(id.stream, which))
      //the fifo enable is off in a corner
      , enabled(id.cword(which ? 0x10 : 0x24), which ? which : 7) {
      //#nada
    }

    INLINETHIS
    operator bool() const {
      return active;
    }

    INLINETHIS
    void clear() const {
      clearit=true;
    }

    bool flagged() const {
      if (active) {
        clear();
        return true;
      } else {
        return false;
      }
    }

    INLINETHIS // NOLINT(misc-unconventional-assign-operator)
    void operator=(bool beOn) const {
      enabled = beOn;
    }
  };

  struct Stream {
    //the four interrupts are also present in operation, but operation can't change as a group while enabled but these can.
    const InterruptBit dmError;
    const InterruptBit error;
    const InterruptBit half;
    const InterruptBit done;
    const InterruptBit fifoError;

    const ControlStruct<Dma::Operation> operation;//assign this from a DmaOperation
    const ControlBit enabled;
    struct FifoControl {
      unsigned threshold: 2;
      unsigned enabled: 1;//else "direct mode"
      unsigned level: 3;
//interrupt is done elsewhere
    };
    const ControlStruct<FifoControl> fifo;//

    const ControlItem<uint16_t> count;
    const ControlWord peripheralAddress;
    const ControlWord memoryAddress;
    const ControlWord otherMemoryAddress;

    constexpr Stream(StreamId id) : // NOLINT(google-explicit-constructor)
      dmError(id, 1)
      , error(id, 2)
      , half(id, 3)
      , done(id, 4)
      , fifoError(id, 0)
      , operation(id.cword(0x10))
      , enabled(id.cword(0x10), 0)
      , fifo(id.cword(0x24))
      , count(id.cword(0x14))
      , peripheralAddress(id.cword(0x18))
      , memoryAddress(id.cword(0x1C))
      , otherMemoryAddress(id.cword(0x20)) {
    }

    void stop() const {
      enabled = false;
    }

    void configure(Dma::Operation op, FifoControl fifoing) const;

    /** configures and prepares a  device to/from memory transfer */
    const Stream &transfer(AddressCaster target, AddressCaster source, unsigned quantity, Operation op, FifoControl fifoing) const;

    /** if the configuration never changes then use this for device to memory transfers */
    const Stream &transfer(AddressCaster target, AddressCaster source, unsigned quantity) const;

    void clearFlags() const;
  };
} //end namespace Dma

//ST's standard symbols:
#define  DMA1_Stream0_irq 11
#define  DMA1_Stream1_irq 12
#define  DMA1_Stream2_irq 13
#define  DMA1_Stream3_irq 14
#define  DMA1_Stream4_irq 15
#define  DMA1_Stream5_irq 16
#define  DMA1_Stream6_irq 17

#define  DMA1_Stream7_irq 47

#define  DMA2_Stream0_irq 56
#define  DMA2_Stream1_irq 57
#define  DMA2_Stream2_irq 58
#define  DMA2_Stream3_irq 59
#define  DMA2_Stream4_irq 60
#define  DMA2_Stream5_irq 68
#define  DMA2_Stream6_irq 69
#define  DMA2_Stream7_irq 70

//use the following where a decimal number of the interrupt request is expected.
#define DmaIrq(luno, stream) MACRO_wrap( DMA ,luno, MACRO_wrap( _Stream , stream , _irq ) )

// end of pieces needed for DmaIrq macro}
#pragma clang diagnostic pop
#pragma clang diagnostic pop