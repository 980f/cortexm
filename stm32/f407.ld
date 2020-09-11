/*
stm32F407:
512k flash
128K ram at the normal address
64K ram at 1000_0000. Probably will only use for "fast" segments
*/

MEMORY {
  UNPLACED_SECTIONS (wx) : ORIGIN = 1024M, LENGTH = 0
  SRAM (wx) : ORIGIN = 512M , LENGTH = 128K
  FLASH (rx) : ORIGIN = 128M , LENGTH = 512K
}
