#pragma once
#include "eztypes.h"
#include "peripheraltypes.h"

//this guy moves around too much, live with gratuitous warnings: #include "../ignoresignwarnings.h"

/** many, but not all, cortex devices put peripheral control registers in the 0x4000 space, and bitband all of that to 0x4200.
 * "bitband" is ARM's term for mapping each bit of the lower space into a 32bit word in the bitband region. 
This replaces a 3-clock operation that is susceptible to interruption into a one clock operation that is not. That is important if an ISR is modifying the same control word as main thread code.
*/

inline constexpr u32 bandShift(u32 byteOffset){
  return byteOffset<<5;
}

/** @return bitband address for given bit (default 0) of @param byte address.
this assumes that the byte address ends in 00, which all of the ones in the st manual do.
*/
constexpr unsigned bandit(unsigned byteAddress, unsigned bitnum = 0){
  //bit gets shifted by 2 as the underlying mechanism inspects the 32bit word address only, doesn't see the 2 lsbs of the address.
  //5: 2^5 bits per 32 bit word. we expect a byte address here, so that one can pass values as read from the stm32 manual.
  //0xE000 0000: stm32 segments memory into 8 512M blocks, banding is within a block
  //0x0200 0000: indicates this is a bitband address
  //bit to lsbs of address |  byteaddress shifted up far enouhg for address space to go away | restore address space | bitband indicator.
  return ((bitnum << 2) | bandShift(byteAddress) | (byteAddress & 0xE0000000) | 0x02000000);
}


/** @return bitband address for given bit (default 0) of @param byte address.
this assumes that the byte address ends in 00, which all of the ones in the st manual do.
*/
constexpr volatile unsigned *bandFor(unsigned byteAddress, unsigned bitnum = 0){
  return atAddress ((bitnum << 2) | bandShift(byteAddress) | (byteAddress & 0xE0000000) | 0x02000000);
}

/** @return bitband address for given bit (default 0) of @param byte address.
this assumes that the byte address ends in 00, which all of the ones in the st manual do.
*/
constexpr volatile unsigned *bandAddress(unsigned * byteAddress, unsigned bitnum = 0){
  return bandFor(unsigned(byteAddress),bitnum);
}

const unsigned PeripheralBand(0x42000000);

