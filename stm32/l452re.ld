/*
the sram can be configured in 2 ways:
160k at the usual start, noting that the last 32k is parity checked
128k at the usual start, plus 32k at 256M, the latter being used for urgent code.

*/

MEMORY {
  UNPLACED_SECTIONS (wx) : ORIGIN = 0x100000000, LENGTH = 0
  FLASH (rx) : ORIGIN = 0, LENGTH = 512K
  SRAM (wx) : ORIGIN = 512M , LENGTH = 160K
}
