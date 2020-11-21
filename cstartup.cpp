#include "cruntime.h" //to validate it, herein lay default implementations of it.

/**
This file is the reset handler.

You may have to tell the linker that the reset entrypoint is "cstartup".
Rowley's build used the name "reset_handler".

*/


/* CMSIS uses this function for doing things that must precede C init, such as turning on IO ports so that configuration commands in constructors function.
You can't use any static *object* but you can use static initialized plain old data */
void SystemInit(); // __USE_CMSIS __USE_LPCOPEN

#pragma GCC diagnostic ignored "-Wmain"

int main(); // entry point

//the linker script creates and initializes these constant structures, used by cstartup()
const extern RamInitBlock __data_segment__;//name coordinated with cortexm.ld
//zero initialized ram
const extern RamBlock __bss_segment__;  //name coordinated with cortexm.ld
//run all code in .init sections.
const extern InitRoutine __init_table__[];//name coordinated with cortexm.ld
//atexit stuff
const extern InitRoutine __exit_table__[];//name coordinated with cortexm.ld

/** this implementation trusts the linker script to null terminate the table */
void run_table(const InitRoutine *table) {
  while (InitRoutine routine = *table++) {
    (*routine)();
  }
}

// instead of tracking #defined symbols just dummy up the optional routines:
[[gnu::weak]] void SystemInit() {
//  return;
}

#include "peripheraltypes.h" //
const extern RamInitBlock __CCM_Vectors__;//name coordinated with cortexm.ld
void vectors2ram() {
  __CCM_Vectors__.go();
  SFRint<unsigned *,0xE000ED08> ()=__CCM_Vectors__.ram.address;
}
/** Reset entry point. The chip itself has set up the stack pointer to top of ram, and then the PC to this. It has not set up a frame pointer.
 */
extern "C" //to make it easy to pass this to linker sanity checker.
[[noreturn]] //we don't need no stinking stack frame (no params, no locals) gnu::naked generates a warning, so we dropped it even though it causes a few useless instructions to be emitted.
void cstartup(void) {
  // initialize static variables
  __data_segment__.go();
  // Zero other static variables.
  __bss_segment__.go();
  // a CMSIS hook: can move to __init_table__ sorted to be first.
  SystemInit(); // stuff that C++ construction might need, like turning on hardware modules (e.g. GPIO group inits)

  run_table(__init_table__); //includes running constructors for static objects
  //incorporated by linker into our __init_table__:  __libc_init_array(); // C++ library initialization 

  //
#if VECTORSINRAM ==1
  vectors2ram();
#endif
  main();
  //execute destructors for static objects and do atexit routines.
  run_table(__exit_table__);

  generateHardReset(); // auto restart on problems, design your system to tolerate spontaneous power cycles on fatal firmware error
}

[[maybe_unused]] // stack pointer is set to end of ram via linker script, gets followed by:
void (*resetVector)() __attribute__((section(".vectors.1"))) = cstartup;
// rest of table is in nvic.cpp, trusting linker script to order files correctly per the numerical suffix on section name



//trying to get good assembler code on this one :)
[[noreturn]] void generateHardReset(){
  //maydo: DSB before and after the reset
  //lsdigit: 1 worked on stm32, 4 should have worked but looped under the debugger.
  ControlWord airc(SCB(0x0C));
  unsigned pattern=0x5FA'0005U | (airc & bitMask(8,3));//retain priority group setting, JIC we don't reset that during startup
  do {//keep on hitting the bit until we reset.
    airc=pattern;
    //probably should try 5 instead of bit 3 above in case different vendors misread the arm spec differently.
  } while (true);
}


#ifdef __linux__  //for testing compiles with PC compiler etc.
const RamInitBlock __data_segment__={0,{0,0}};
const RamBlock __bss_segment__={0,0};
const InitRoutine __init_table__[]={nullptr};
const InitRoutine __exit_table__[]={nullptr};
const unsigned __stack_limit__(0);
#else



#endif

