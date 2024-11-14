#pragma once

/** calling this will generate a hard reset if the stack has overlapped the heap or static data allocations. */
extern "C" void stackFault();
//heapless
extern "C" void free(...);
extern "C" void *malloc(...);