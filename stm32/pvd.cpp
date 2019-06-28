#include "pvd.h"

PVD::PVD() :
    APBdevice(1, 28), //
    dcb(*reinterpret_cast<PVDdcb *>(blockAddress)) {
  init();
}

void PVD::setThreshold(int mV) {
  mV -= 2200;
  mV /= 100; //programmable in 10ths of a volt.
  if (mV < 0) {
    mV = 0;
  }
  if (mV > 7) {
    mV = 7;
  }
  dcb.threshold = mV;
  dcb.enablePVD = 1;
}

static unsigned pup_spun = 0xDEADBEEF; //4debug
bool PVD::waitForGood(unsigned spinner) {
  while (spinner-- > 0) {
    //usually should break before count down completes.
    if (!isLow()) {
      pup_spun = spinner; //4debug
      return true; //power is good.
    }
  }
  pup_spun = 0xBAAD;
  return false; //power is low.
}
