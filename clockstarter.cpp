#include "clocks.h"
#include "systick.h"

void ClockStarter::go() const {
  if(coreHertz){
    //todo:M actually honor it!
  } else {
    warp9(intosc);
  }
  if(sysHertz){
    SystemTimer::startPeriodicTimer(sysHertz);
  }
}

ClockStarter::ClockStarter(bool intosc, Hertz coreHertz, Hertz sysHertz):
  intosc(intosc),
  coreHertz(coreHertz),
  sysHertz(sysHertz)
{
  //const constructor
}
