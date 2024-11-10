/*
BIG WARNING
The STM32F407 will NOT allow vectors to be read from the CCM, and that may be true of all vendors.
This is hinted at by comments that the bit that differs from 2000'0000 and 0000'0000 is used to pick the bus the vectors are read from.
This has been confirmed by the systick fault vectoring to FFFF'FFFF, causing the processor to freeze, at least when a debugger is attached.

If putting vectors in RAM is valuable they will need to be put into the normal ram.

*/


SECTIONS {
  /* __Ram_Vectors__.go() will copy the vectors to the start of SRAM, you will then have to write to VTOR to configure use of them. */
.vectorsFast : { /* this initializes an instance of what cstartup.cpp calls 'struct RamInitBlock' */
    LONG(LOADADDR(.vectors))
    LONG(ORIGIN(SRAM))
    LONG((SIZEOF(.vectors)+3)>>2);
  } >FLASH
  __Ram_Vectors__ = LOADADDR(.vectorsFast);
}
