#ifndef CLOCKGENERATOR_H
#define CLOCKGENERATOR_H

namespace SAM {

/** there are 3 clock generators, for feeding external devices. */
void clockOutPut(unsigned whichOf3, bool enable);

/** there is a fast internal RC oscillator, good enough for running the clock setting code.
 * 4,8,12 are the allowed values. 0x20 bits 6,5,4 = (MHz/4)-1, set bit 3 to enable. */
//NYI: void fastRC(unsigned Mhz);

/** @param prescale is 3 bits, 111 for /3 else log2(some power of 2)
 * @param source is slow, main, pllA, Upll. if Upll then the uppl/2 bit is set herein, else it is cleared.
*/
void masterClock(unsigned prescale, unsigned source);

/** enable or disable a device clock. The base frequency is clockRate(1) */
void deviceClock(unsigned pid,unsigned enable);

}


#endif // CLOCKGENERATOR_H
