#pragma once
/**
  * Software Implemented I2C Master
  * Drive the I2C bus as a single master, 400kHz slaves that never stretch the clock.
  * (recovered from cpp file somehow named h)
  */
#include "gpio.h"
class I2C {
unsigned pinbase;
OutputPin sda;
InputPin sdaRead;
InputPin sclRead;
ControlBool sdaOff;
OutputPin scl;
ControlBool sclOff;
I2C(int luno, bool alt1){
  pinbase = 2 + 4 * luno + (alt1 ? 2 : 0);
}

void configurePins(const Port&sdaPort, int sdaPinNumber, const Port&sclPort, int sclPinNumber){
  Pin sdapin(sdaPort, sdaPinNumber);
  Pin sclpin(sclPort, sclPinNumber);
  sda = sdapin.DO(Port::PinOptions::Slew::fastest); //will toggle pin function from normal to OD via sdaOff control
  sda = 1;
  sdaOff = sdapin.highDriver();
  scl = sclpin.DO(Port::PinOptions::Slew::fastest); //don't support slave clock stretching at high speed
  scl = 1;
  sdaRead = sdapin.reader();
  sclRead = sclpin.reader();
} /* configurePins */

void configurePins(bool ){
  configurePins(PB, pinbase + 1, PB, pinbase);
}

/** clock and data should already be definitely high before calling this. */
void SendStart( ){
  scl = 1; //just in case...
  sda = 1;
  sdaOff = 0;
  sda = 0; //1->0 while clock high == start
  nanoTicks(500);
  scl = 0;
  nanoTicks(300); //st is sluggish, use 300 instead of 260
}
//deferred these defines to where we won't be tempted to corrupt them for special uses, like starts.
#define FloatSDA          scl = 0; sda = 1; sdaOff = 1
#define DriveSDA(whatto)  scl = 0; sda = whatto; sdaOff = 0

/** SendStop - sends an I2C stop, releasing the bus.
  * by leaving this a subroutine call we get the required deadtime between stop and start.
  */
void SendStop( ){
  DriveSDA(0);
  scl = 1;
  nanoTicks(500); //generous stop
  sda = 1; //data 0->1 while clock high == stop.
  //single master: *sdaOff=1;
  nanoTicks(500); //guaranteed silence between transactions
}

/**
  * wiggleBits - sends one byte of data to an I2C slave device.
  * clock is left high, ack bit is still on the wire upon return.
  * @return the ack bit from the byte transfer.*/
bool wiggleBits( uint8_t A ){
  unsigned bitPicker = 1 << 7;

  do {
    DriveSDA((A & bitPicker) ? 1 : 0);
    bitPicker >>= 1;
    nanoTicks(500); //clock low
    scl = 1;
    nanoTicks(400); //atmel is sluggish
  } while(bitPicker);
  FloatSDA;  // Release data line for acknowledge.
  nanoTicks(500); //500 was coming out as 370!
  scl = 1;
  nanoTicks(500); //gross delay to see if ack is just late.
  return sdaRead; //Set status for no acknowledge.
} /* wiggleBits */

/**
  * on entrance sda is being driven
  * on exit sda is being driven to the ack value*/
u8 readBits(bool nackem){
  int BitCnt = 8;
  uint8_t A = 0; //assignment for debug

  FloatSDA;
  do {
    scl = 0;
    nanoTicks(450); //read access time
    scl = 1;
    A <<= 1;
    A |= sdaRead;
    nanoTicks(400); //atmel is sluggish
  } while(--BitCnt > 0);
  DriveSDA(nackem);
  nanoTicks(400); //atmel is slow, should be 260
  scl = 1;
  return A;
} /* readBits */



bool notify(bool happy){
  SendStop();
  if(inprogress) {
    inprogress->onTransferComplete(happy);
    inprogress = 0;
  }
  return happy;
}

bool transact(Transactor&transact){
  inprogress = &transact;
  bool justProbing = !inprogress->moreToWrite();
  justProbing &= !inprogress->moreToRead();
  if(justProbing) {
    SendStart();
    return notify(!wiggleBits(inprogress->device | 1));
  }
  if(inprogress->moreToWrite()) { //if something to write
    SendStart();
    if(wiggleBits(inprogress->device )) { //send address w/write selected
      //address not acknowledged
      return notify(false);
    }
    while(inprogress->moreToWrite()) {
      if(wiggleBits(inprogress->nextByte())) {
        //extremely rare to get here
        return notify(false);
      }
    }
    DriveSDA(1); //finish ack's clock and take back control of sda
    nanoTicks(250); //setup for repeated start.
  }
  if(inprogress->moreToRead()) { //is something to read
    SendStart(); //often a repeated start
    if(wiggleBits(inprogress->device | 1)) { //send address w/read selected
      //address not acknowledged
      return notify(false);
    }
    bool more; //a bit of nastiness just to NACK the last byte we read, a stupid part of the I2C spec since STOP already carries that info.
    do {
      u8 *target = &inprogress->nextByte();
      more = inprogress->moreToRead();
      *target = readBits(!more);
    } while(more);
    DriveSDA(1);
    nanoTicks(250); //setup for stop
  }
  return notify(true);
} /* transact */
//end of file
