/*
simplified linker script for bare metal flash execution only

This should be included *after* a script that defines MEMORY sections SRAM and FLASH.

This takes advantage of gcc emitting data alignment commands in the input files, so we don't have to redundantly implement them herein.

For the c startup initialization it trades off number of inline constants (register load immediates) with
putting those constants explicitly in the rom and inlining only a pointer thereto. The compiler was able to do a load-multiple versus separate load-immediates at the point of use,
an infinitesimal gain in rom usage but less than half as many magic names.

The building of the vector table is shared between linker generated stack top and the vector table proper being
declared elsewhere (@see 980f's nvic.cpp) using the same facility that 'init' attributes uses.
I used SORT where others don't bother so that the vector table can be developed in separate files,
such as stack init and reset vector where the code using them is, and the others in the nvic module.

Some people (CMSIS- looking at you) conditionally include some special calls around the c startup
(useful for things like powering up parts of the chip that need to be on before c-structs for accessing them are constructed),
I have implemented that via weak references to a routine that devolves into a "bx lr". Compare that to CMSIS having #define symbols managing that,
one extra instruction and the time for a useless call and return gain you not having to manage #define symbols.

Some people have multiple initialization call tables, I pack them into one segment and null terminate it
which eliminates the need for a slew of shared symbols and counting. I may have unintentionally altered the
priority, the point of the multiple lists was to allow for some control over order in which initialization
routines execute, which is done via the order of input declarations. Number them via attributes and sort!

---
The following dance is done for some sections:
    KEEP (*(SORT(.data.*)))
    KEEP (*(.data))
This allows one to build a table from pieces in different translation units.
For each such table use __attribute__((section(".ramorrom.tablename.somenumber")) applied to each entry.
Reserve somenumber=0 for the first entry to the table, and 999999 or zzzzz or the like for marking the end of the table.
A simple unsigned allocated at each of those magic values gives you (with minor syntax) the address of the first and last entries in the table.
A good example would be a set of logical timers that are run off of a common hardware timer. Each module needing such a timer includes the header with
the table definition (and a macro to make the section declaration easy) and then declares a module level entity with the related attribute.
The numbers following the tablename allow for ordering of the table, although most of the time a common value should be used and only give a few entities 
a custom value to put them earlier or later in the table.
---
No special provision has been made for a ram vector table. The compiler can align a user built array of function pointers
and user code can assign to the vector table system control register to enable it.

I initially left off destructor and other 'atexit' functionality, my only 'atexit' is to generate a hard reset. I've added that mostly back in but it is untested.

I have left off a no-init data section, that is a dangerous technology with many different possible implementations.
The stm32 non-volatile data has holes in it beyond the ability of the linker to readily compensate for. Not zeroing some ram
that will programmatically be loaded from a non-volatile device is a minor performance burden for keeping this script generic.

I have made it convenient to swap around the order of the sections without having to rework explicit references to the others.

The cmake makefiles mess with working directory, so I had to add '-L parent-directory-to-my-ld-files' to the linker command string for it to find included ld files such as this one.

*/

SECTIONS {
/* interrupt table.   Needs KEEP directive as these functions will only be referenced by this table. */
  .vectors ORIGIN(FLASH):{
    LONG( ORIGIN(SRAM)+LENGTH(SRAM))         /* set initial stack pointer to top of sram */
    KEEP (*( SORT(.vectors.*)  .vectors))    /* followed by reset and other handlers */
  } >FLASH

/* normal code */
  .text : {
    *(.text .text.*)
     /* 980f commented out segments produced by features he doesn't want to be using, some of which are not available on cortexM? parts */
     /**(.glue_7t .glue_7 .gnu.linkonce.t.* .gcc_except_table .ARM.extab* .gnu.linkonce.armextab.*)  */
  } >FLASH

/* things that should be called before main():  
 Needs KEEP directive as these functions will only be referenced by this table.*/
  .init : {
/* code that initializes static data via startup-time computation: */
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
 /* static constructors after C lib initializations: */
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    LONG(0)  /*mark end of table, instead of having code for decrementing a count */
  } >FLASH
  __init_table__ = LOADADDR(.init);

/* exit table, although a good mcu program never exits */
  .exit : {
    *(SORT(.dtors.*))       /* explicit destructions, 980f thinks this should follow the plain dtors but this copies some legacy ld files */
    *(.dtors)               /* implicit destructions (nameless objects) */
    *(.fini_array)          /* general exit stuff */
    *(SORT(.fini_array.*))  /* particular exit stuff, 980f thinks this should precede the plane fini_array */
    LONG(0)                 /* mark end of table, instead of having code for decrementing a count */
  } >FLASH
__exit_table__ = LOADADDR(.exit);

/* fully const'ed data, note that some things declared const end up in .data despite valiant attempts to compile time compute values. */
  .rodata :{
    /** our table segments were not referenced explicitly and therefore were discarded when placed into rodata. 
        Eventually the 'used' attribute was applied to the C++ declarations, but we shall keep the KEEP in case something reverts.
        We can't just keep all rodata's because the libraries are chock full of such sections */
    KEEP( *(SORT(.table.*)))
    *(SORT(.rodata.*))
    *(.rodata)
    /* shouldn't be getting these, want to know if we do:  .gnu.linkonce.r.* */
  } >FLASH

/* zero initialized data */
  .bss (NOLOAD) : {/* NOLOAD is gratuitous, but is a reminder that all that is occurring is data allocation.*/
    *(SORT(.bss.*))
    *(.bss)
/* tbss added for F407+ st's hal use, just for errno! Once we understand it we might be able to get rid of it  */
    *(SORT(.tbss.*))
    *(.tbss)
    /* shouldn't be getting these: .gnu.linkonce.b.*) *(COMMON)*/
  } >SRAM

 .tbss (NOLOAD) : {/* NOLOAD is gratuitous, but is a reminder that all that is occurring is data allocation.*/
    
  } >SRAM

  .startupClear : {/* the following initializes an instance of what cstartup.cpp calls 'struct RamBlock' */
    LONG(ADDR(.bss))
    LONG((SIZEOF(.bss)+3)>>2)  /* word count instead of bytes, startup takes advantage of compiler forced alignment to do 4 bytes per cycle. */
  } >FLASH
  __bss_segment__ = LOADADDR(.startupClear);

/* data with constant initializers */
  .data :{
    *(SORT(.data.*))
    *(.data)
    /*.gnu.linkonce.d.*) */
  } >SRAM AT>FLASH

  .startupCopy : { /* this initializes an instance of what cstartup.cpp calls 'struct RamInitBlock' */
    LONG(LOADADDR(.data))
    LONG(ADDR(.data))
    LONG((SIZEOF(.data)+3)>>2)
  } >FLASH
  __data_segment__ = LOADADDR(.startupCopy);

/* diagnostic, or feed into a runtime stack check routine */
  .findEnd :{ /* cute trick to get the total SRAM used. This trick keeps us from having to move the __stack_limit__ assignment into whichever ram block becomes the last.*/
    __stack_limit__ = .;  /* if SP is ever below this value we are corrupting memory. */
  } >SRAM

  /* value to someday feed into start of data segment should we ever figure out how to put the stack first so that we get a fault instead of wild behavior */
  __stack_max__ = LENGTH(SRAM) - (__stack_limit__ - ORIGIN(SRAM));

/* theoretically one can have a memory declaration for each peripheral,
  in C code declare an object for each as extern,
  have a section name for each and an attribute in front of data declaration
  and then the linker can provide addresses for access.
  That also entails laying out a struct to match the hardware and given the number of 16 bit wide peripherals that can be quite a pain.
  Without automated code generation that is all quite painful and error prone, and the code isn't any faster or smaller than using constexpr
  address calculation code (when the compiler is allowed to inline it).

  Nonetheless, if one wanted to do all that segment magic one could include a file here with the section per peripheral stuff,
  and likewise have an include in the device.ld  declaring all the peripheral blocks.
*/

/* The following fragment is copied from the AC6 auto generated ld file, not yet sure what it does. */
  /* Remove information from the compiler libraries */
/* 10aug2019 libc.a linker error hunt
/DISCARD/ : {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
*/
}
