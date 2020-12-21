//so many defects in hardware that we couldn't do more than one operation per reset.
//this code is now too stale to copile, need to crib from stm HAL.
#include "i2c.h"
#include "minimath.h"
#include "gpio.h"

#if I2C_TRACE>0
#include "buffer.h"
static I2C::Stage stages[I2C_TRACE];
static Indexer <I2C::Stage> stageTrace(stages, sizeof(stages));
void I2C::setStage(I2C::Stage stageCode){
  stage = stageCode;
  if(stageTrace.hasNext()) {
    stageTrace.next() = stageCode;
  }
}
#endif

static int dbgCount = 0; //will use for 'ignore count' for breakpoints.

I2C::I2C(int luno, bool alt1): APBdevice(1, 20 + luno), b(reinterpret_cast <I2C_BAND *> (getBand())), dcb(reinterpret_cast <I2C_DCB *> (getAddress())), eventIrq(29 + 2 * luno), errorIrq(30 + 2 * luno){
  inprogress = 0;
  pinbase = 2 + 4 * luno + (alt1 ? 2 : 0);
  stage=Stage::Idle;
}

void I2C::configurePins(bool withSmb){
  Pin sda(B, pinbase + 1);
  Pin scl(B, pinbase);
  *sda.writer() = 1;
  *scl.writer() = 1;
  if(pinbase == 8) {
    theAfioManager.b->i2c1 = 1;
  }
  sda.FN(50, true); //todo:3 set edge rate by frequency.
  scl.FN(50, true);
  if(withSmb && pinbase == 10) { //only i2c2 supports smb interrupt
    Pin(B, 12).DI('U');
  }
} /* configurePins */

void I2C::clearHW(void){ //grrr, soft reset also clears config, why does it exist separate from module generic reset?
  setStage(Idle);
  b->Reset = 1; //clear the consequences of any trash created by error
  b->Reset = 0; //... or debugging :)
  dbgCount = 0;
}

void I2C::init400k(void){ //todo:3 parameters for fast and a kiloHertz
  init();
  clearHW();
#ifdef I2C_TRACE
  stageTrace.rewind();
#endif
  //cr1 as apb.init sets it up is fine
  int apbRate = getClockRate();
  dcb->cr2 = apbRate / 1000000; //keeps all interrupt enable stuff disabled
  //compute the two fast options, choose the highest
  int r1 = rate(apbRate, (25 * 400000)); //@36 = 4, 4*25 = 100 clocks = 360kHz
  int r0 = rate(apbRate, (3 * 400000)); //@36 = 30, 30*3 = 90 clocks = 400kHz
  if(25 * r1 > 3 * r0) {
    dcb->ccr = (3 << 14) /* fast 9:16*/ | r1;
  } else {
    dcb->ccr = (2 << 14) /* 400kHz 1:2*/ | r0;
  }
  //maximum clock risetime: 3/1000000 = 3e-9 in integers without overflowing
  dcb->riseTime = 1 + quanta(apbRate * 3, 10000000); //300ns: ~11 clocks at 36MHz apb clock.
  //alh: suspect the +1 given in the documents is due to them truncating rather than rounding up.

  //enable interrupts at NVIC:
  eventIrq.prepare(); //NB: simple enable() often generates an interrupt even when all interrupts are masked, dammit! (Busy was true)
  errorIrq.enable();
} /* init */

void I2C::setPriority(u8 level){
  eventIrq.setPriority(level);
  errorIrq.setPriority(level);
}

bool I2C::transact(Transactor&transact){
  if(!busFree()) {
    return false;
  }
  inprogress = &transact;

  b->dataIE = 0; //defer until after the address, don't know why it matters unless some signal is lingering after a previous transaction.
  b->eventIE = 1;
  b->errorIE = 1;

  b->enable = 1; //and enable operation (you should already have configured the pins)

  start();
  return true;
} /* transact */

void I2C::notify(bool happy){
  if(inprogress) {
    inprogress->onTransferComplete(happy);
    inprogress = 0;
  }
}


void I2C::dataIsr(void){ //stop,start,address, addr10,byte transfer finished. and also rxne and txe.
  Flags sr;

  if(dbgCount && --dbgCount == 0) {
    dbgCount = 1; //4breakpoint
  }
  sr.sr1 = dcb->sr1; //can only read this register once, reading it modifies it.
  sr.sr2 = dcb->sr2; //not sure about sr2 so treat it the same (note in figure 235 of stm manual suggest that the read needs to be done.)

  switch(sr.wad) {
  case 0x030001: //I2C_EVENT_MASTER_MODE_SELECT:                 /* EV5 */  03 0001
    if(!inprogress) { //WE didn't ask for a start so kill stuff
      stop();
      return;
    }
    setStage(Addressing); //hardware starts with write to data register, need to set state sooner in case we are single stepping.
    //setting the READ/!write bit presuming that device address is properly set for writes.
    //nothing to write AND nothing to read menas checking for presence, which must be done via a write with no payload.
    dcb->data = inprogress->device | (!inprogress->moreToWrite() && inprogress->moreToRead());
    break;
  case 0x070082: //I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED: //07 0082  /* BUSY, MSL, ADDR, TXE and TRA flags */
    b->dataIE = 1;
    setStage(Sending);
    if(inprogress->moreToWrite()) {
      dcb->data = inprogress->nextByte();
    } else { //just checking for device presence
      stop(); //device presense check.
      notify(true);
    }
    break;
  case 0x070080: //I2C_EVENT_MASTER_BYTE_TRANSMITTING  /* TRA, BUSY, MSL, TXE flags */
    if(inprogress->moreToWrite()) {
      dcb->data = inprogress->nextByte();
    } else {
      b->dataIE = 0; //turn off "buffer empty" interrupt when the last byte is sent.
    }
    break;
  case 0x070084: //I2C_EVENT_MASTER_BYTE_TRANSMITTED  /* TRA, BUSY, MSL, TXE and BTF flags */
    //byte shifted out, and we didn't enable data interrupt.
    if(inprogress->moreToRead()) {
      start(); //restart for Paged Read operation.
    } else {
      stop(); //normal simple write termination
      notify(true);
    }
    break;
  case 0x030002: //I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:  /* BUSY, MSL and ADDR flags */
    //apparently reading of the status triggers the data read and so we had better exit the isr before the byte shifts in else we miss an interrupt.
    setStage(Receiving);
    b->Ack = 1; //we always read a bunch, never just one or two bytes
    b->dataIE = 1;
    break;
  case 0x030044: //I2C_EVENT_MASTER_BYTE_RECEIVED_BTF /* BUSY, MSL, RXNE, BTF */
  //#usually get the '44 version. Don't know if we ever get the '40 version.
  case 0x030040: { //I2C_EVENT_MASTER_BYTE_RECEIVED  /* BUSY, MSL and RXNE flags */
    if(inprogress->moreToRead()) {
      inprogress->nextByte() = dcb->data;
      unsigned int NumbOfBytes = inprogress->readRemaining();
      if(NumbOfBytes <= 1) { //'<' to deal with bugs.
        b->Ack = 0;  //for devices which must be nacked on last byte.
        dbgCount = 1;
        stop(); //doesn't execute until after a byte in progress finishes.
      }
    } else { //we read 1 more than we should have, oh well.
      b->Ack = 0;  //for devices which must be nacked on last byte.
      stop(); //doesn't execute until after a byte in progress finishes.
      notify(true); //stop has already been issued due to their wierd pipelining.
    }
  }
  break;
  case 0x000044: //empirically discovered.
  //#under debug conditions get the '44 version. Don't know if we ever get the '40 version.
  case 0x000040: //I2C_EVENT_MASTER_BYTE_RECEIVED_NO_BUSY: /* RxNE */
    /* In case the STOP is generated on the bus and the last byte remains to be read */
    if(inprogress->moreToRead()) {
      inprogress->nextByte() = dcb->data;
      //and here is where we note we are done
      notify(true); //stop has already been issued due to their wierd pipelining.
    } else {
      notify(true); //stop I said.
    }
    break;
  case 0x000000: //happens although manufacturer denies it.
    if(stage == Stopping) { //WAG:
      setStage(Idle);
      notify(true); //stop has already been issued due to their wierd pipelining.
    }
  default:
    // need to try to ignore false interrupts:// stop();
    //notify(false);
    break;
  } /* switch */
} /* dataisr */

void I2C::errorIsr(void){
  Flags sr;

  sr.sr1 = dcb->sr1; //can only read this register once, reading it modifies it.
  sr.sr2 = dcb->sr2; //not sure about sr2 so treat it the same (note in figure 235 of stm manual suggest that the read needs to be done.)

  if(sr.nacked) { //typical cause is device not present, NB: eeprom's disappear when they are busy.
    b->nacked = 0;
    stop();
    notify(false); //this disallows writing to non-existent devices as a debug tool, //todo:1 let inprogress decided whether to stop.
  }
  if(sr.busError) {
    b->busError = 0;
    stop(); //probably should reset and re-initialize.
    notify(false);
  }
  if(sr.lostArbitration) {
    b->lostArbitration = 0;
    stop(); //we are the sole master so this was a major hardware glitch.
    notify(false);
  }
  //the remaining aren't used by master-only, but are here so we can see if they fire off.
  if(sr.overrunError) {
    b->overrunError = 0;
  }
  if(sr.pecError) {
    b->pecError = 0;
  }
  if(sr.sclTimedout) {
    b->sclTimedout = 0;
  }
  if(sr.smbAlerting) {
    b->smbAlerting = 0;
  }
} /* errorIsr */
//end of file
