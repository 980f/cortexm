#pragma once

#define CRUNTIME_H
/** parts of the c startup that might be useful to a running program */

extern "C" [[gnu::naked,noreturn]] void generateHardReset();

/** these structs are created via LONG(...) directives in the ld file.
   That way only one symbol needs to be shared between the ld file and this file for each block.*/
struct RamBlock {
  unsigned int *address;
  unsigned int length; //number of unsigneds
  /** NB: this startup presumes 32 bit aligned, 32bit padded bss.
   *  Does anyone remember what BSS originally meant? Nowadays it is 'zeroed static variables' */
  void go(void)const{
    unsigned *target=address;
    for(unsigned count=length;count> 0;--count) {//## ;count-->0; took more code bytes and execution than this, without -o3.
      *target++ = 0;
    }
  }
};

struct RamInitBlock {
  const unsigned int *rom;
  RamBlock ram;
  /** NB: this presumes 32 bit aligned, 32bit padded structures, compared to common usage of memcpy this moves 4 bytes at a time, without the overhead of testing whether that can be done. */
  void go(void)const {
  //FYI: using locals is slightly faster than member, and lets us const the structure
    const unsigned int *source=rom;
    unsigned *target=ram.address;
    for(unsigned length=ram.length;length> 0;--length) {//## ;length-->0; took more code bytes and execution than this. 
      *target++ = *source++;
    }
  }
};

/** every type of code driven initialization is invoked through a simple void fn(void)*/
typedef void (*InitRoutine)(void) ;
/** this implementation trusts the linker script to null terminate the table.
 * To insert something into the table you must put it into a section .ctors[.*]. lower number/alpha after the 'ctors' run earlier.
 * the 'initialization priority logic' uses such sections to control startup sequencing, and doesn't work without linker support.
 */
void run_table( const InitRoutine * table);

/** generate hard reset if the stack has overflowed */
void stackFault();
