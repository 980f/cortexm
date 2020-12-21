#include "pairedoneshots.h"


PairedOneshots::PairedOneshots(int luno, int longerLuno, int shorterLuno): Timer(luno), shorter(*this, shorterLuno), longer(*this, longerLuno){}

void PairedOneshots::init(int refFrequency){
  apb.init();
  b->oneshot = 1; //don't loop around once passing both thresholds. we want a single event on a timeout, not a repeated reminder.
  setPrescaleFor(refFrequency);
  //todo:1 init the actual pins
}

void PairedOneshots::initCC(CCUnit&cc, int ticks){
  cc.setmode(0x10);
  cc.setTicks(ticks);
  //todo:1 make this conditional:
  cc.pin().FN();
}
/** expects to be called from an isr. Checks and lcears source flags
  * 2 for long fired, 1 for short fired, 0 for neither is fired*/
int PairedOneshots::whichFired(void){
  if(longer.happened()) {
    longer.clear();
    shorter.clear(); //bas
    return 2;
  } else if(shorter.happened()) {
    shorter.clear();
    return 1;
  } else {
    //the above are the only interrupt sources we intentionally set.
    return 0;
  }
} /* whichFired */
