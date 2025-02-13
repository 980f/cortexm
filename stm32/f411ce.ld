/*
stm32F411xE:
512k flash
128K ram at the normal address
*/

MEMORY {
  UNPLACED_SECTIONS (w) : ORIGIN = 4096M, LENGTH = 0
  SRAM (w) : ORIGIN = 512M , LENGTH = 128K
  FLASH (rx) : ORIGIN = 128M , LENGTH = 512K
}
