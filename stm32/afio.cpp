#include "afio.h"
#include "bitbasher.h"

#if DEVICE >= 200
AfioManager theAfioManager InitStep(InitHardware-1); //just before ports and pins

//struct AfioEvent {
//  unsigned evPin:4;
//  unsigned evPort:3; //letter for port -'A'
//  unsigned evEnable:1;
//  unsigned :24;
//};

void AfioManager::remap(unsigned bitOffset,bool setit){
  auto bitter=theAfioManager.bit(0x04,bitOffset);
  bitter=setit;
}

void AfioManager::selectEvent(const PinDeclaration &pin){
  beEnabled();
  //4 per word, 0th at 8
  ControlField gangof4(registerAddress(8+(pin.bitnum&~3U)),4*(pin.bitnum&3U),4);//4 fields per register, 4 bytes per register= ignore 2 lsbs of port's bit number
  u32 value(pin.portIndex);//port A is slot 2.
  gangof4=value;//2 lsbs of port's bitnumber select 4 bit field
}

#endif
