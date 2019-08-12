#include "steppercontrol.h"

enum StepParam {SH,SV,SA,};

StepperControl::StepperControl(void):HasSettings("HVA"){
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

bool GasPedal::setParams(ArgSet&args){
  set(cruise ,args);
  set(start ,args,cruise);
  set(accel ,args);
  return isModified();
}

void GasPedal::getParams(ArgSet &args)const{
  args.next() = cruise;
  args.next() = start;
  args.next() = accel;
}

/////////////////////////
bool Homer::setParams(ArgSet&args){
  set(widest ,args);
  set(unused ,args);
  set(offset ,args);
  return isModified();
}

void Homer::getParams(ArgSet&args)const{
  args.next() = widest;
  args.next() = unused;
  args.next() = offset;
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

int StepAccess::numParams()const{
  return 2;
}

bool StepAccess::setParams(ArgSet &args){
  Settable::set(position ,args);
  Settable::set(hz,args,0.0);
  return isModified();
}

void StepAccess::getParams(ArgSet &args)const{
  args.next() = position;
  if(hz!=0){
    args.next()=hz;
  }
}

bool StepperControl::setParam(ID fieldID, ArgSet&args){
  switch(pMap(fieldID)) {
  default:
    return false;
  case SV:
    also(v.setParams(args));
    break;
  case SH:
    also(h.setParams(args));
    break;
  case SA:
    also(a.setParams(args));
    break;
  } /* switch */
  return true;
} /* setParam */

void StepperControl::touch(ID fieldID){
  switch(pMap(fieldID)) {
  case SH:
    h.also(true);
    break;
  case SV:
    v.also(true);
    break;
  case SA:
    a.also(true);
  }
  HasSettings::touch(fieldID);
}

bool StepperControl::getParam(ID fieldID, ArgSet&args)const{
  switch(pMap(fieldID)) {
  default:
    return false;
  case SV:
    v.getParams(args);
    break;
  case SH:
    h.getParams(args);
    break;
  case SA:
    a.getParams(args);
    break;
  } /* switch */
  return true;
} /* getParam */

bool StepperControl::propagate(StepperControl &image,bool diagsToo){
  bool changed=false;
  if(h.differed(image.h)) {
    post('H');
    changed=true;
  }
  if(v.differed(image.v)) {
    post('V');
    changed=true;
  }
  if(diagsToo&& a.differed(image.a)) {//#order is critical
    post('A');//this interferes with the tray being at a position, only can send when try level of logic is in diagnostic mode.
    changed=true;
  }
  return changed;
}

bool MotorReport ::setParams(ArgSet &args){
  set(motionCode,args);
  set(homingStage,args);
  set(hasHomed,args);
  set(location,args);
  set(target,args);
  also(markReport.setParams(args));
  return isModified();
}

void MotorReport ::getParams(ArgSet &args)const{
  args.next()=motionCode;
  args.next()=homingStage;
  args.next()=hasHomed;
  args.next()=location;
  args.next()=target;
  markReport.getParams(args);
}
