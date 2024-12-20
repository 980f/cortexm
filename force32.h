//
// Created by andyh on 2/6/21.
//

#pragma once

#include "bitbasher.h"
/** some peripherals  have a requirement of aligned 32 bit write.
 * gcc will optimize a write or field insert to only manipulate the bytes changing, which for these memory spaces leads to bad things.
 *
 * This "unsigned workalike" class alows you to manipulate the pattern for such a beast, and writes it on exit of scope.
 * */
class Force32 {
  unsigned &wordwide;

public:
  unsigned cache;

public:
  /** this records the address AND reads the present value */
  Force32(unsigned &wordwide):wordwide(wordwide),cache(wordwide){}

  /** you can explicitly flush, useful when in a loop in scope */
  void flush() {
    wordwide = cache;
  }

  /** flushes to real memory on exit of scope*/
  ~Force32() {
    flush();
  }

  operator unsigned () {
    return cache;
  }

  Force32& operator |=(unsigned rhs) {
    cache |=rhs;
    return *this;
  }

  Force32& operator &=(unsigned rhs) {
    cache &=rhs;
    return *this;
  }

  Force32& set(unsigned msb,unsigned lsb,unsigned pattern) {
    mergeField(cache,pattern,msb,lsb);
    return *this;
  }

};
