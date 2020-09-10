#pragma once

#include "eztypes.h"

/**
system timer service (a not so fast timer)

Somewhere you must declare:
void SystemTickServer();

which will get called FROM AN ISR periodically.

*/

//time that isn't forever, but good for any one action's duration.
using SysTicks = uint32_t;
//time that won't roll over
using SysTime = uint64_t;
//functions to call on the SystemTick
using SystemTicker = void (*)();

namespace SystemTimer {

/** start ticking at the given rate.
  * presumes system clock is already programmed and that it is the clock source*/
  void startPeriodicTimer(unsigned persecond);

/** time since last rollover (<1 ms if 1kHz, even shorter at higher sysfreq), only suitable for spinning in place for a short time.
NB: this is NOT in ticks, it is in probably 8MHz increments, but that may differ for some clock configurations.*/
  SysTicks snapTime();

/** much longer time range but an expensive call, in units of system tick, call secondsForTicks()*/
  SysTicks snapTickTime();

/** much much longer time range, with a range greater than the life of the instrument., call secondsForLongTime()*/
  SysTime snapLongTime();

  double secondsForTicks(SysTicks ticks);

  double secondsForLongTime(SysTime ticks);

/** ticks necessary to get a delay of @param sec seconds, 0 if sec is negative */
  SysTicks ticksForSeconds(float sec);

/** ticks, but if ms is negative then you get 0 */
  SysTicks ticksForMillis(int ms);

/** ticks, but is us is negative then you get 0 */
  SysTicks ticksForMicros(int us);

/** ticks necessary to get a periodic interrupt at the @param hz rate*/
  SysTicks ticksForHertz(float hz); //approximate since we know a divide is required.
}

