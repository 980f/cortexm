#include "afio.h"
#include "bitbasher.h"

AfioManager theAfioManager InitStep(InitHardware-1); //just before ports and pins

//struct AfioEvent {
//  unsigned evPin:4;
//  unsigned evPort:3; //letter for port -'A'
//  unsigned evEnable:1;
//  unsigned :24;
//};

AfioManager ::AfioManager():
  APBdevice(APB2, 0),
  b(*reinterpret_cast <volatile AfioBand *> (bandAddress)),
  remap(registerAddress(4)){
  init();
}

void AfioManager::selectEvent(const Pin &pin){
  //4 per word, 0th at 8
  ControlField gangof4(registerAddress(8+(pin.bitnum&~3U)),4*(pin.bitnum&3U),4);//4 fields per register, 4 bytes per register= ignore 2 lsbs of port's bit number
  u32 value(pin.port.slot-2);//port A is slot 2.
  gangof4=value;//2 lsbs of port's bitnumber select 4 bit field
}
