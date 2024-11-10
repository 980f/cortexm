#pragma once

/* GNU gcc specific syntax for some special instructions.
 * This has devolved into MNE() for such things as WFE and WFI.  MNE(WFE)  and MNE(WFI), there is no need to make global symbols for things that rarely appear more than once in any program */

#if __linux__
//then just compiling for syntax checking.
#define MNE(mne)
#else
//#define SINGLEOP(mne)  INTRINSICALLY unsigned __ ## mne(unsigned value){uint32_t result; __asm volatile (# mne " %0, %1" : "=r" (result) : "r" (value));  return result; }
// the above despite all the inline keywords was made into a callable function,so:
#define MNE(mne) __asm volatile (# mne)

#endif
