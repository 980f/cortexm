#include "clocks.h"
#include "systick.h"

ClockStarter::ClockStarter(bool intosc, u32 coreHertz, u32 sysHertz):
  intosc(intosc),
  coreHertz(coreHertz),
  sysHertz(sysHertz)
{
  if(coreHertz){
    //todo:M actually honor it!
  } else {
    warp9(intosc);
  }
  if(sysHertz){
    SystemTimer::startPeriodicTimer(sysHertz);
  }
}
