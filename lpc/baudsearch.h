#ifndef BAUDSEARCH_H
#define BAUDSEARCH_H

#include <stdint.h>

#include "hook.h"

/**
there are two dividers and a twiddler aka "fractional rate divider"
the first divider is 1:255, the second 1 to 65535, the twiddle factors 4 bits each.

the target is 16 * the nominal baudrate.

More rules:
1. 1 <=MULVAL<= 15
2. 0 <=DIVADDVAL<= 14
3. DIVADDVAL< MULVAL

baud = pclk / (16.0*divider)*(1.0+(div/mul)));

search space:
the DIVADDVAL and MULVAL can only produce a factor between 1 and 2.
The integer divider must yield a value within a factor of 2 of the desired value.

*/

struct BaudSearch{
  /** search result */
  static BaudSearch nearest;
  /** target baud rate */
  static double desiredbaud;
  /** uart peripheral clock, usually the cpu clock. */
  static double pclk;
  /** search given the static members */
  static void findBaudSettings();
  static Hook<const BaudSearch &> snoop;


  /** baud computed from other members */
  unsigned baud;
  /** uart divider register */
  uint16_t divider;
  /** DIVADDVAL */
  uint8_t div;
  /** MULVAL */
  uint8_t mul;

private:
  void seek();
  void apply();
  double error()const;
  void improve();

};


#endif // BAUDSEARCH_H
