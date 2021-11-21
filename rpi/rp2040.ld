/*
pi pico core 0
256k ram at 2000_0000
 8k more right after it so we merge them.


Flash will be accessed via XIP:
XIP at 1000_0000.
You will need to create a different instance of this file for each board that has a different sized QSPI flash.

*/

MEMORY {
  UNPLACED_SECTIONS (wx) : ORIGIN = 1024M, LENGTH = 0
  SRAM (wx) : ORIGIN = 512M , LENGTH = 264K
  FLASH (rx) : ORIGIN = 256M , LENGTH = 2M
}

