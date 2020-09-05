#include "cruntime.h" //to validate it, herein lay default implementations of it.

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
[[gnu::weak, gnu::optimize(3)]] void SystemInit() {
//  return;
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
  // a CMSIS hook:
  SystemInit(); // stuff that C++ construction might need, like turning on hardware modules (e.g. 980f LPC::GPIO::Init())

  run_table(__init_table__);
  //incorporated by linker into our run_init:  __libc_init_array(); // C++ library initialization (includes constructors for static objects)
  main();
  //execute destructors for static objects and do atexit routines.
  run_table(__exit_table__);

  generateHardReset(); // auto restart on problems, design your system to tolerate spontaneous power cycles on fatal firmware error
}

// stack pointer is set to end of ram via linker script, gets followed by:
void (*resetVector)() __attribute__((section(".vectors.1"))) = cstartup;
// rest of table is in nvic.cpp, trusting linker script to order files correctly per the numerical suffix

//todo: separate out stackFault into its own file, for those who won't use it nor wtf.
#include "wtf.h"  //error routine, a place to share a breakpoint for trouble.

extern unsigned const __stack_limit__;//created in linker script
void stackFault() {
  unsigned here;
  if (&here <= &__stack_limit__) {//todo: should add a few so that we can call wtf without risk
    wtf(99999999);
    generateHardReset();
  }
}

#ifdef __linux__  //for testing compiles with PC compiler etc.
const RamInitBlock __data_segment__={0,{0,0}};
const RamBlock __bss_segment__={0,0};
const InitRoutine __init_table__[]={nullptr};
const InitRoutine __exit_table__[]={nullptr};
const unsigned __stack_limit__(0);
#else

//-nostartupfiles seems to have gotten rid of the need for these stubs.
////destructor failure stuff, but ours won't ever actually fail
//extern "C" int __aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle){
//  return int(&destructor)+int(object)+int(dso_handle);//stupid code to get rid of gratuitous warnings.
//}
/** sometimes pure virtual functions that aren't overloaded get called anyway,
  * such as from extended classes prophylactically calling the overloaded parent,
    or constructors calling their pure virtual members */
//extern "C" void __cxa_pure_virtual(){  /* upon call of pure virtual function */
//  wtf(100000); /* ignore it, but have a place for a breakpoint */
//}


//these guys get linked in when you have explicit destructors, even if those destructors are "=default".
//... that keeps on rehappening due to the warning of 'have virtual functions but not virtual destructor' inspiring the creation of pointless destructors.
//... which I guess is a potential burden- not having destructors on classes that happen to be used statically, but might be useful 'automatically' as well.
//extern "C" {
//  void __aeabi_atexit(){
//    wtf(-1);
//  }
//  void __dso_handle(){
//    wtf(-2);
//  }
//}

#endif

