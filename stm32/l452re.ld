/*
the sram has two chunks and allows two views of one of those chunks.
SRAM1 (per ST naming) is 128K at the usual start,
SRAM2 is 32K at both the CCM start and following SRAM1.
SRAM2 is parity checked and may be accessible during DMA to the other RAM.

Below I am configuring SRAM to be just SRAM1 and calling their SRAM2 "CCM" to match the F407.
*/

MEMORY {
  UNPLACED_SECTIONS (wx) : ORIGIN = 0x100000000, LENGTH = 0
  FLASH (rx) : ORIGIN = 0, LENGTH = 512K
  SRAM (wx) : ORIGIN = 512M , LENGTH = 128K
  CCM (wx)  : ORIGIN = 256M , LENGTH = 32K
}
