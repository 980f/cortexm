#ifndef MSSERVICE_H
#define MSSERVICE_H

/** millsecond timer service, using systick timer.
 * In addition to including the headers for the base classes used this also instantiates the systick service table,
 * a linker built array of things to call with each timer tick.
 */

#include "systick.h"
using namespace SystemTimer;
#include "polledtimer.h"
#include "tableofpointers.h"
#define RegisterPolledTimerWithSysTick MakeRef(SystemTicker,PolledTimerServer);


#endif // MSSERVICE_H
