#ifndef FEATURE_H
#define FEATURE_H

#include "peripheraltypes.h"

#include "boolish.h"
/** many internal things have a set of registers for control and status.
 *  one register is 'set ones corresponding'
 *  another register is 'clear ones corresponding'
 *  a 3rd is a read back of the bits, and sometimes allows full write
 *  a 4th is sometimes also bit oriented, others just loosely related to the others.
 */
namespace SAM {

/**
 * @param invertedSense: the designers show a total disregard for consistency in the relationship between the readback and the set/clear addresses */

template<unsigned base,unsigned mask,bool invertedSense=false> struct Feature : public BoolishRef {

  void operator =(bool enableit) const {
    if(enableit) {
      *atAddress(base + 4 * invertedSense) = mask;
    } else {
      *atAddress(base + 4 * (1 - invertedSense)) = mask;
    }
  }

  operator bool() const {
    return (*atAddress(base+2*4)&mask)!=0;
  }

  void set4th(bool on){
    unsigned address=base+3*4;
    if(on){
      *atAddress(address)|=mask;
    } else {
      *atAddress(address)&=~mask;
    }
  }

  bool get4th(){
    return bit(*atAddress(base+3*4),mask);
  }

  /**occasionally the 4th member is independent of the bit: */
  unsigned &fourth() const {
    return *atAddress(base+3*4);
  }

};

/** many sets of feature bits have a register which write protects them.
e.g.: PIO: piobase , lockword E4, key 0x504D43
*/
template <unsigned block, unsigned lockword, unsigned key> struct FeatureLock {
  unsigned chunk(unsigned which)const {
    return block + which<<4;//groups of 4 registers, 4 bytes each.
  }
  ///lock bit:
  bool operator =(bool lock)const{
    *atAddress(block+lockword)=key<<8|lock;
    return lock;
  }

  operator bool()const{
    return *atAddress(block+lockword) &1;
  }

};


} // namespace SAM

#endif // FEATURE_H
