#ifndef STEPPERCONTROL_H
#define STEPPERCONTROL_H

#include "hasSettings.h"
#include "cheapTricks.h"
#include "markreport.h"
struct GasPedal :public Settable{
  double start;
  double cruise;
  double accel;
  GasPedal(void);

  int numParams()const{
    return 3;
  }
  bool setParams(ArgSet&args);
  void getParams(ArgSet&args)const;

  /**number of steps to achieve full speed*/
  double acceleration(void);
  bool isValid(){
    return cruise!=0 && start!=0 &&(cruise!=start&&accel>0);
  }
};

/**
  * wheel geometry is resendable with each homing request.
  * between + across should be at least as big as the separation between stopping points,
  * and can be bigger since both should be the largest of their kind which aren't typically adjacent.
  */
struct Homer:public Settable {
  int unused; //
  int widest; //width of biggest index mark
  int offset; //per tray accumulated error in mark vs. sample window

  Homer():
    unused(0),
    widest(0),
    offset(0) {
  }

  int numParams()const{
    return 3;
  }
  bool setParams(ArgSet&args);
  void getParams(ArgSet&args)const;

  bool notConfigured(){
    return widest==0 ;
  }
  void setOffset(int off){
    set(offset,off);
  }

};

struct StepAccess:public Settable {
  int position; //absolute position in steps, negative allowed for circularity and home offset.
  double hz;

  bool run;
  bool clockwise;
  u32 ticks;

  int numParams()const;
  bool setParams(ArgSet&args);
  void getParams(ArgSet&args)const;

  void setPosition(int pos){
    set(position,pos);
  }
};


class StepperControl : public HasSettings {
public:
  StepAccess a;
  Homer h;
  GasPedal v;
  StepperControl(void);
private:
public:
  bool setParam(ID fieldID, ArgSet&args);
  void touch(ID fieldID);
  bool getParam(ID fieldID, ArgSet&args)const;
  /** trigger a recentering even if c.position and h stuff doesn't change.*/
  void retrigger(bool h2);
  void setTarget(int step);
  /** @return whether a message has been made pending */
  bool propagate(StepperControl &other,bool diagsToo);
};

struct MotorReport:public Settable {
  int motionCode;
  int homingStage;
  bool hasHomed;
  int location;
  int target;
  MarkReport markReport;

  MotorReport():
    motionCode(),
    homingStage(),
    hasHomed(),
    location(),
    target(),
    markReport() {
  }

  int numParams()const {
    return 5+markReport.numParams();
  }
  bool setParams(ArgSet&args);
  void getParams(ArgSet&args)const;
};


#endif // STEPPERCONTROL_H
