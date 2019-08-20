#include "steppercontrol.h"

StepperControl::StepperControl(void){
  a.position = 0;
  a.hz=0;
}
/////////////////////////
GasPedal::GasPedal(void) {
  start = cruise = 0;
  accel = 0;
}

/**number of steps to achieve full speed*/
double GasPedal::acceleration(void){
  return ratio((cruise - start), accel);
}

/////////////////
void StepperControl::retrigger(bool h2){
  also(true);
  v.also(true);//doesn't trigger activity so doesn't cost much
  if(h2){
    h.also(true);
  }
}

void StepperControl::setTarget(int step){
  a.setPosition(step);
}


