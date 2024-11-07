#pragma once
/** the processor's debug hardware controls
 * At the moment only F103's content is here, need to do an ifdef switch on processor define.
 *
 */

//#include "stm32.h"

//DBGMCU_CR, Only 32-bit access supported

enum DebugControlBit {
  WhileSleeping, //Hclk stays running, is connected to Fclk
  WhileStopped, //H&F clocks run by rc oscillator, power is left on to digital stuff
  WhileStandingBy, //H&F clocks run by rc oscillator
  skip3,
  skip4,
  TraceOutputEnable,
  TraceMode0,
  TraceMode1,
  WatchdogStop,
  WindowWatchdogStop,
  Timer1Stop,
  Timer2Stop,
  Timer3Stop,
  Timer4Stop,
  Can1Stop,
  I2c1TimeoutStop,
  I2c2TimeoutStop,
  Timer8Stop,
  Timer5Stop,
  Timer6Stop,
  Timer7Stop,
  Can2Stop,
  //22..31 not used.

};

inline void setDebugControl(enum DebugControlBit cbit){
  *reinterpret_cast <u32 *> (0xE0042004) |= 1 << cbit;
}
//end of file.
