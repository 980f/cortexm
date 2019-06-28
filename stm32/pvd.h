#ifndef PVD_H
#define PVD_H

#include "stm32.h"

class PVD: public APBdevice {
  struct PVDdcb{
    unsigned lowPowerOnStop:1;//drop regulator power on stop (not standby), slows down recovery.
    unsigned doStandby:1;//else stops
    unsigned clearWakeup:1;
    unsigned clearStandby:1;
    unsigned enablePVD:1; //1 turns on the comparator
    unsigned threshold:3;
    unsigned enableRtcWrites:1;
    unsigned :32-9;
    unsigned Wakeup:1;
    unsigned Standby:1;
    unsigned powerIsLow:1;
    unsigned :5;
    unsigned enableWakeupEvent:1;

  } &dcb;
public:
  PVD();
  void setThreshold(int mV);
  bool isLow(){
    return dcb.powerIsLow;
  }
  /** return whether power became good*/
  bool waitForGood(unsigned spinner);
};

#endif // PVD_H
