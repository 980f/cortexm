/*
stm32F411xE:
512k flash
128K ram at the normal address
*/

MEMORY {
  UNPLACED_SECTIONS (wx) : ORIGIN = 1024M, LENGTH = 0
  SRAM (wx) : ORIGIN = 512M , LENGTH = 128K
  FLASH (rx) : ORIGIN = 128M , LENGTH = 512K
}

