

  /* __CCM_Vectors__.go() will copy the vectors to the start of CCM, you will then have to write to VTOR to configure use of them. */

.vectorsFast : { /* this initializes an instance of what cstartup.cpp calls 'struct RamInitBlock' */
    LONG(LOADADDR(.vectors))
    LONG(ORIGIN(CCM))
    LONG((SIZEOF(.vectors)+3)>>2);
  } >FLASH
  __CCM_Vectors__ = LOADADDR(.vectorsFast);

