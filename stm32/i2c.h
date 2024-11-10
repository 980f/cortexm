#pragma once
//#include "buffer.h"
#include "stm32.h"
#include "nvic.h"
//define in compiler options: #define I2C_TRACE 0
/**
  * module usage:
  *
  * Reference the appropriate TakeI2cIrqsXX
  *
  * configurePins and one of the init's
  *
  * enable PEC if you want a transaction to tack on a checksum to every block.
  * on read the PEC check is reported on the transaction object.
  *
  * SLAVE NOT IMPLEMENTED!
  *
  * DMA removed as just not relevant for a master when so much diddling needs to be done for 'paged read'
  *
  */
#define TakeI2cIrqs1(myi2c)             \
  ObjectInterrupt(myi2c.dataIsr(), 31); \
  ObjectInterrupt(myi2c.errorIsr(), 32)

#define TakeI2cIrqs2(myi2c)             \
  ObjectInterrupt(myi2c.dataIsr(), 33); \
  ObjectInterrupt(myi2c.errorIsr(), 34)

; //#this semi colon fixes a bug in the qt source code indenter.
/**
  * reading the SR1 has non-trivial side effects, so init a local Flags structure with dcb->sr1 then interpret the bits.
  * however, write using the BAND else you have to remember to copy the Flags back to the sr1 (but never the sr2).
  */
union Flags {
  u32 wad;
  struct {
    u16 sr1;
    u16 sr2; //only need 8 but this makes for a clean 32 bit value.
  };
  struct {
    unsigned int started : 1; //"cleared by reading sr1 then writing DR"
    unsigned int addressed : 1;
    unsigned int byted : 1;
    unsigned int highAddressSent : 1;

    unsigned int stopped : 1;
    unsigned int skip5 : 1;
    unsigned int dataAvailable : 1; //clear by reading DR, not set by address bytes.
    unsigned int canSend : 1; //transmit register empty.

    unsigned int busError : 1; //unexpected start or stop, implies serious glitch or wild device.
    unsigned int lostArbitration : 1; //some other master collided with us, and won.
    unsigned int nacked : 1; //must clear via software.
    unsigned int overrunError : 1; //slaving: when not clock stretching and clock moves

    unsigned int pecError : 1;
    unsigned int skip13 : 1;
    unsigned int sclTimedout : 1;
    unsigned int smbAlerting : 1;

    unsigned int mastering : 1;
    unsigned int busy : 1;
    unsigned int imTransmitter : 1; //slaving
    unsigned int skip3 : 1;

    unsigned int generalCalling : 1;
    unsigned int SMBDEFAULT : 1;
    unsigned int SMBHOST : 1;
    unsigned int addressedAsSecondary : 1;
  };
};


struct I2C_DCB {
  unsigned int cr1;
  unsigned int cr2;
  unsigned int ownAddress[2]; //NB: must set bit 14 (these should be structs) example code suggests that bit is an enable.
  volatile unsigned int data;
  volatile u16 sr1;
  u16 waste1;
  volatile u8 sr2;
  u8 pec;
  u16 waste2;
  unsigned int ccr;
  unsigned int riseTime;
};

struct I2C_BAND {
  unsigned int enable;
  unsigned int smbus;
  unsigned int ignored2;
  unsigned int smbHost;

  unsigned int arpenable;
  unsigned int PECEnable;
  unsigned int ackGeneralCall;
  unsigned int disableClockstretch;

  unsigned int Start; //hw will send a start signal. supposedly waits if a stop is still in progress
  unsigned int Stop;  //hw waits until current byte finished before sending a stop signal
  unsigned int Ack;   //set for reading, maybe triggers the read?
  unsigned int ackPec;

  unsigned int sendPec;
  unsigned int Alert;
  unsigned int res14;
  unsigned int Reset; //release the bus

  unsigned int skip1[16 + 8]; //16 bit perif, skip freq register

  unsigned int errorIE;
  unsigned int eventIE;
  unsigned int dataIE;
  unsigned int dmaEnable;
  unsigned int last; //dma related, nacking last byte of dma read.
  unsigned int skip3[32 - 13]; //rest of cr2
  unsigned int skip2[32 + 32 + 32]; //skip own addresses,data register
  //sr1:  //see struct Flags for cautions.
  unsigned int started; //"cleared by reading sr1 then writing DR"
  unsigned int addressed;
  unsigned int byted;
  unsigned int highAddressSent;
  unsigned int stopped;
  unsigned int skip5;
  unsigned int dataAvailable; //clear by reading DR, not set by address bytes.
  unsigned int canSend; //transmit register empty.

  unsigned int busError; //unexpected start or stop, implies serious glitch or wild device.
  unsigned int lostArbitration; //some other master collided with us, and won.
  unsigned int nacked; //must clear via software.
  unsigned int overrunError; //slaving: when not clock stretching and clock moves
  unsigned int pecError;
  unsigned int skip13;
  unsigned int sclTimedout;
  unsigned int smbAlerting;
  //  unsigned int sr1high[16]; //16 bit device in 32 bit space.
  //  //sr2
  //  volatile unsigned int mastering;
  //  volatile unsigned int busy;
  //  volatile unsigned int imTransmitter; //slaving
  //  unsigned int skip3;
  //  volatile unsigned int generalCalling;
  //  unsigned int SMBDEFAULT;
  //  unsigned int SMBHOST;
  //  volatile unsigned int addressedAsSecondary;
  //  // rest of sr2,
  //  //  pec[8];
  //  //  skkip[16];
  //  //  ccr[32];
  //  //  riseTime[32];

};

/**
  * first use is to master read and write an eeprom.
  */
class I2C : public APBdevice {
public:
  //debug is hopeless without these:
  enum Stage { Idle, Starting, Addressing, Sending, Restarting, Receiving, Stopping } stage;
  void setStage(I2C::Stage stageCode);

private:
  I2C_BAND *b;
  I2C_DCB *dcb;

  Irq eventIrq;
  Irq errorIrq;

  int pinbase;
public:
  struct Transactor {
    bool done;
    bool succeeded;
    int device;

    virtual bool moreToWrite(void) = 0;
    virtual bool moreToRead(void) = 0;
    virtual unsigned int readRemaining(void) = 0; //due to pipelining we have to get advanced warning of end of iteration.

    virtual u8&nextByte(void) = 0;
    //return true to restart, presumably after having setup the next operation.
    virtual void onTransferComplete(bool successfully){
      succeeded = successfully;
      done = true;
    }

    void prepare(int devaddress){
      succeeded = done = false;
      device = devaddress;
    }
  };
private:
  Transactor *inprogress;
public:
  //had to publicize as the isr itself isn't informing us of read-completed
  void notify(bool happy = false);
private:
  inline void start(void){
    setStage(stage == Idle ? Starting : Restarting);
    b->Start = 1;
  }

  inline void stop(void){
    setStage(Stopping);
    b->Stop = 1;
  }

  void clearHW(void);
public:
  I2C(int luno, bool alt1);

  void configurePins(bool withSmb = false);
  void init400k(void);

  inline bool busFree(void){
    return inprogress == 0 /*&& hardware indicates done?*/;
  }

  bool transact(Transactor&transact);
  void setPriority(u8 level);
public:
  //only because isr linkage requires it.
  void dataIsr(void);
  void errorIsr(void);
};
