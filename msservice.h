#ifndef MSSERVICE_H
#define MSSERVICE_H

/** millsecond timer service, using systick timer.
 * In addition to including the headers for the base classes used this also instantiates the systick service table,
 * a linker built array of things to call with each timer tick.
 */

#include "systick.h"
using namespace SystemTimer;
#include "sharedtimer.h"
#include "tableofpointers.h"
//the following macro must be uinvoked in a single place in your build
#define RegisterPolledTimerWithSysTick MakeRef(SystemTicker,SharedTimerServer);


#endif // MSSERVICE_H
