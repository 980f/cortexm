#ifndef DEBUGIO_H
#define DEBUGIO_H
/**
  * rowley crossstudio debug printer works via storing some constants in your program
  * source then calling a routine which has a hard breakpoint set upon it (built into
  * the program binary rather than using the debug hardware).
  *
  * upon recognizing that particular breakpoint they use debug access to get the printf
  * arguments, collect the data, then format it on the PC running the IDE.
  *
  * If you enable their debug stuff then if the jtag debugger is not installed your program will
  * freeze hard if you forget to check debug_enabled():
  */

#if ALH_SIM == 0
#include <cross_studio_io.h>  //for debug_printf stuff.
#define printf if(debug_enabled()) debug_printf
#else
#define printf(...)
#endif

#else
#include_next <debugio.h>
#endif // DEBUGIO_H
