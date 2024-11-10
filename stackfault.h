#ifndef STACK_FAULT_H
#define STACK_FAULT_H

#ifdef __cplusplus
 extern "C" {
#endif

/** calling this will generate a hard reset if the stack has overlapped the heap or static data allocations. */
void stackFault();

#ifdef __cplusplus
}
#endif

#endif
