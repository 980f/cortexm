#pragma once

#include "stm32.h"
#include "nvic.h"

struct Uart: public APBdevice {
  const unsigned zluno;   //zluno is ST's nomenclature -1

  struct DCB {

    volatile u32 SR; //for parallel read,
    volatile u32 DR; //up to 9 bits are meaningful
    u32 BRR; //16 bits, manual is full of BS over fraction's and mantissa's

    //CR1
    unsigned int SBK : 1; //send break: triggers a single break char (LIN mode, not sure what it does when not in LIN mode)
    unsigned int RWU : 1; //something to do with wakeup feature...
    unsigned int RE : 1; //receiver enable
    unsigned int TE : 1; //transmitter enable
    unsigned int IDLEIE : 1;
    unsigned int RXNEIE : 1;
    unsigned int TCIE : 1;
    unsigned int TXEIE : 1;
    unsigned int PEIE : 1;

    unsigned int PS : 1; //odd else even parity.
    unsigned int PCE : 1; //parity bit enabled
    unsigned int WAKE : 1; //on address else on idle
    unsigned int M : 1; //TRUE= 9 BIT ELSE 8
    unsigned int UE : 1; //manual garbled, this appears to be a clock enable.
    unsigned int : 32 - 14;

    //CR2
    unsigned int ADD : 4;  //multi-drop unit address
    unsigned int : 1;
    unsigned int LBDL : 1;  //11 else 10 bits for a break in LIN mode.
    unsigned int LBDIE : 1;
    unsigned int : 1;
    unsigned int LBCL : 1; //spi mode: one more clock at end of chunk.
    unsigned int CPHA : 1; //spi mode: experiment till it works :)
    unsigned int CPOL : 1; //spi mode: idle value of clock line
    unsigned int CLKEN : 1;

    //this requires coding an enum unsigned int STOP:2;//
    unsigned int ShorterStop : 1; //reduce by half a bit
    unsigned int LongerStop : 1; // add a full bit.
    unsigned int LINEN : 1; //
    unsigned int : 32 - 15;

    //CR3
    unsigned int EIE : 1;
    unsigned int IREN : 1;
    unsigned int IRLP : 1;
    unsigned int HDSEL : 1;
    unsigned int NACK : 1;
    unsigned int SCEN : 1;
    unsigned int DMAR : 1;
    unsigned int DMAT : 1;
    unsigned int RTSE : 1;
    unsigned int CTSE : 1;
    unsigned int CTSIE : 1;
    unsigned int : 32 - 11;

    unsigned int PSC : 8;
    unsigned int GT : 8;
    unsigned int : 32 - 16;
  };

  volatile DCB &dcb;

  struct Band {
    //beware that using these bits has side effects that may clear others of them!
    volatile unsigned int parityError;
    volatile unsigned int framingError;
    volatile unsigned int noiseError;
    volatile unsigned int overrunError;
    volatile unsigned int idleLine;
    volatile unsigned int dataAvailable;
    volatile unsigned int transmitCompleted;
    volatile unsigned int transmitBufferEmpty;
    volatile unsigned int LINBreakDetected;
    volatile unsigned int clearToSendChanged;
    unsigned int srwaste[32 - 10];

    u32 DR[32];
    u32 BRR[32];

    //CR1
    unsigned int sendBreak; //send break: triggers a single break char (LIN mode, not sure what it does when not in LIN mode)
    unsigned int RWakeUp; //something to do with wakeup feature...
    unsigned int enableReceiver; //receiver enable
    unsigned int enableTransmitter; //transmitter enable
    unsigned int IDLELineIE;
    unsigned int dataAvailableIE;
    unsigned int transmitCompleteIE;
    unsigned int transmitAvailableIE;
    unsigned int parityErrorIE;

    unsigned int parityOdd; //odd else even parity.
    unsigned int parityEnable; //parity bit enabled
    unsigned int WAKEonAddress; //on address else on idle
    unsigned int _9bits; //TRUE= 9 BIT ELSE 8
    unsigned int enable; //manual garbled, this appears to be a baudrate generator enable.
    unsigned int cr1waste[32 - 14];

    //CR2
    unsigned int cr2rsv[5];
    unsigned int LB11bitBreak;  //11 else 10 bits for a break in LIN mode.
    unsigned int LBDetectedIE;
    unsigned int cr2rsv2;
    unsigned int LBclocked; //spi mode: one more clock at end of chunk.
    unsigned int Cphase; //spi mode: experiment till it works :)
    unsigned int CPolarity; //spi mode: idle value of clock line
    unsigned int CLKEnable;

    unsigned int halfStop; //reduce by half a bit
    unsigned int doubleStop; // add a full bit.
    unsigned int LINEnable; //
    unsigned int cr2waste[32 - 15];

    //CR3
    unsigned int EIE;
    unsigned int IREnable; //infra red pulse shaping enabler
    unsigned int IRLowPower; //infra red pulse shaping detail.
    unsigned int HDSELect;
    unsigned int NACK;
    unsigned int SCEN;
    unsigned int dmaReception;
    unsigned int dmaTransmitter;
    unsigned int RTSEnable; //enables feature, doesn't drive the pin
    unsigned int CTSEnable; //seems gratuitous, versus ignoring it if you don't have one.
    unsigned int CTSIE; //interrupts on any change, you get to decide what the polarity is in your response code
    //no more bit stuff

  };

  volatile Band &b;

  /** the status register clears on read, so we must read it then check the bits in the cached read.
   * No doc exists as to what happens if we bit band read single bits, but reason says that a resetting read is done. */
  union Status {
    struct  {
      unsigned int parityError : 1;
      unsigned int framingError : 1;
      unsigned int noiseError : 1;
      unsigned int overrunError : 1;
      unsigned int idleLine : 1;
      unsigned int dataAvailable : 1;
      unsigned int transmitCompleted : 1;
      unsigned int transmitBufferEmpty : 1;
      unsigned int LINBreakDetected : 1;
      unsigned int clearToSendChanged : 1;
    };
    unsigned flags;
  };

  const Irq irq;
  //unsigned int altpins;//not const so that we can dynamically attach to different pins. If we normally could const it then we could create different objects and hope that our program logic doesn't get too confusing.

  constexpr Uart(unsigned stluno): APBdevice(busForUart(stluno), slotForUart(stluno))
  , zluno(stluno - 1)
  , dcb(*reinterpret_cast<DCB *>(blockAddress))
  , b(*reinterpret_cast<Band *>(bandAddress))
  , irq((irqForUart(stluno))){
  //not touching hardware as doing nothing here lets us constexpr construct when we know the stluno at compile time.
}


  /** we init the handshakes to "not used" as they are nearly useless, as well as having atrociously wrong names.
   * The RTS is nearly useless as it glitches for one bit time at every stop bit (103RB), which makes many remote senders fail. */
  void handshakers(bool hsout, bool hsin) const;

  /** use this if you are changing the rate while the program is running, else @see setParams
    *  this will disable the uart if the baud rate is changed, you must re-enable it after you have finished other configuration actions. */
  void setBaudrate(unsigned desired) const;

  /** use this one for initial setup.
    * this will disable the uart, you must re-enable it after you have finished other configuration actions.
    * longStop adds a stop bit,
    * shortStop removes half of one.
    * NEO is the parity control: None Even Odd.*/
  void setParams(unsigned baud = 19200U, unsigned numbits = 8, char parityNEO = 'N', bool longStop = false, bool shortStop = false)const; //19200,8,n,1

  /** hard reset then setParams*/
  void reconfigure(unsigned baud = 19200U, unsigned numbits = 8, char parityNEO = 'N', bool longStop = false, bool shortStop = false)const; //19200,8,n,1

  void init(unsigned baud = 19200U, unsigned  numbits = 8, char parityNEO = 'N')const;

  /** part of char time calculation, includes stop and start and parity, not just payload bits */
  unsigned bitsPerByte() const;
  /** bits per second, actual not what you last set :)*/
  unsigned bitsPerSecond() const;
  /** timer ticks required to move the given number of chars. Involves numbits and baud etc.*/
  unsigned ticksForChars(unsigned charcount) const;

  void beReceiving(bool yes = true)const;
 
  void beTransmitting(bool yes = true) const;

  void txDma(bool beRunning) const;

  static constexpr unsigned irqForUart(unsigned stluno) {
    return stluno + (stluno <= 3 ? 36 : stluno <= 5 ? (52-4) : (71-6)) ; //37,38,39  52,53 71
  }

public:
  /*
F103: U1 = 1:14, others on 2:luno+15
F407: U1 = 2:4  U2..5 = 1:lu+15   U6=2:5
*/
#if DEVICE == 103 || DEVICE == 452
  static constexpr BusNumber busForUart(unsigned stluno){
    return stluno==1 ? APB2 : APB1;
  }

  static constexpr unsigned slotForUart(unsigned stluno) {
    return (stluno == 1 ? 14 : (stluno + 15));
  }

#elif DEVICE == 407 || DEVICE==411
  constexpr BusNumber busForUart(unsigned stluno){
    return (stluno == 1 || stluno == 6) ? APB2 : APB1;
  }
  constexpr unsigned slotForUart(unsigned stluno) {
    return (stluno == 1 ? 4 : stluno == 6 ? 5 : (stluno + 15));
  }

#else
#error "you must define DEVICE to something like 103 or 407"
#endif


};

//so far the uart irq slots have been the same over many parts. Nice, but weird.

#define  UART1_irq  37
#define  UART2_irq  38
#define  UART3_irq  39
#define  UART4_irq  52
#define  UART5_irq  53
#define  UART6_irq  71
//po tay toe , po tah toe
#define  USART1_irq  UART1_irq
#define  USART2_irq  UART2_irq
#define  USART3_irq  UART3_irq
#define  USART4_irq  UART4_irq
#define  USART5_irq  UART5_irq
#define  UsART6_irq  UART6_irq
