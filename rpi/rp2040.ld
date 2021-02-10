/*
pi pico core 0
256k ram512k flash
128K ram at the normal addres
64K ram at 1000_0000.

*/

MEMORY {
  UNPLACED_SECTIONS (wx) : ORIGIN = 1024M, LENGTH = 0
  SRAM (wx) : ORIGIN = 512M , LENGTH = 128K
  CCM (wx) : ORIGIN = 256M , LENGTH = 64K
  FLASH (rx) : ORIGIN = 128M , LENGTH = 512K
}

