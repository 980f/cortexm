#ifndef HBRIDGE_H
#define HBRIDGE_H

#include "stm32.h"

/** imported from somewhere else, is limited to adjacent port bits.
implements patterns common to many driver chips.
*/
class DualHalfH : public Boolish {
protected:
  Port::Field output; //the bss+bsr of the port
public:
  /** lsbit is lower number bit in field regardless of polarity*/
  DualHalfH(const Port &port, unsigned lsbit, bool reversed=0);

  /** aka "set the brake" */
  void hold(){
    output = 0;
  }

  void off(){
    output = 3;
  }

  bool isForward(){
    u16 snap = output;//read for debug
    return snap == (reversed?1:2);
  }

  bool isReversing(){
    u16 snap = output;//read for debug
    return snap == (reversed?2:1);
  }

  /** isForward() or isReversing() done optimally (only read hardware once, it is runtime expensive to do so)*/
  bool isActive(){
    u16 snap = output;//read once
    return snap == 1 || snap == 2;
  }

  /**simple forward or back, @return given direction */
  virtual bool operator = (bool beOn) {
    output = (beOn==reversed)?1:2;
    return beOn;
  }

  /**simple forward is active else not. */
  operator bool(void){
    return isForward();
  }

};



#include "systick.h" //for shared timer

/**
  * CyclicTimer is a misnomer for 'retriggerable monostable which knows its time constant'
  */
class BidirectionalThingy : public CyclicTimer {
protected:
  DualHalfH control;
  SimpleDO reducedPower;//the enable pin for those that have one.
public:
  BidirectionalThingy(const Port &port, unsigned lsbit, bool reversed, const Pin * powerpin = 0);
  /** virtual to allow "onChange" activity.*/
  virtual void turn(bool beOn);
  void apply(BidiSetting&bs);
  virtual void onDone(void);
};

/**
  * pulse for on, reverse polarity pulse for off
  */
class BistableRelay : public BidirectionalThingy {
public:
  BistableRelay(const Port &port, unsigned lsbit, bool reversed=0);
  void onDone(void);
};

#endif // HBRIDGE_H
