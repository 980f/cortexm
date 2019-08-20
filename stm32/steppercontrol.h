#ifndef STEPPERCONTROL_H
#define STEPPERCONTROL_H


#include "cheapTricks.h"
struct GasPedal {
  double start;
  double cruise;
  double accel;
  GasPedal(void);

  /**number of steps to achieve full speed*/
  double acceleration(void);
  bool isValid(){
    return cruise!=0 && start!=0 &&(cruise!=start&&accel>0);
  }
};


struct StepAccess {
  int position; //absolute position in steps, negative allowed for circularity and home offset.
  double hz;

  bool run;
  bool clockwise;
  u32 ticks;


  void setPosition(int pos){
    set(position,pos);
  }
};


class StepperControl {
public:
  StepAccess a;
  GasPedal v;
  StepperControl(void);
public:
  /** trigger a recentering even if c.position and h stuff doesn't change.*/
  void retrigger(bool h2);
  void setTarget(int step);

};

struct MotorReport {
  int motionCode;
  int homingStage;
  bool hasHomed;
  int location;
  int target;

  MotorReport():
    motionCode(),
    homingStage(),
    hasHomed(),
    location(),
    target(),
    markReport() {
  }

};


#endif // STEPPERCONTROL_H
