#include "hbridge.h"

DualHalfH ::DualHalfH(const Port&port, unsigned lsbit,): output(2/*gp2MHz*/,port, lsbit, lsbit+1){}



BidirectionalThingy::BidirectionalThingy(const Port&port, unsigned lsbit, const Pin *powerpin): control(port, lsbit), reducedPower(powerpin ? powerpin->DO(): 0){
  reducedPower=1;
  lastRequested=0;
  control.off();
}

void BidirectionalThingy::onDone(){
  reducedPower = 1;
  done=true;
}

void BidirectionalThingy::turn(bool beOn) {
  if(beOn!=lastRequested){
    reducedPower = 0;
    control = (lastRequested = beOn);
    retrigger();
  }
}

void BidirectionalThingy::apply(BidiSetting&bs){
  period = PolledTimer::ticksForSeconds( bs.period);
}

/////////////////////////
BistableRelay::BistableRelay(const Port&port, unsigned lsbit): BidirectionalThingy(port, lsbit){
  //power supplies may not be stable yet, do not act.
}

void BistableRelay::onDone(void){
  control.off();
}
