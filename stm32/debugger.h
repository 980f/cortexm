#pragma once
/** the processor's debug hardware controls
 * At the moment only F103's content is here, need to do an ifdef switch on processor define.
 *
 */

//#include "stm32.h"

//DBGMCU_CR, Only 32-bit access supported
#if DEVICE == 103
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
#else
enum DebugControlBit {
  WhileSleeping, //Hclk stays running, is connected to Fclk
  WhileStopped, //H&F clocks run by rc oscillator, power is left on to digital stuff
  WhileStandingBy, //H&F clocks run by rc oscillator
  skip3,
  skip4,
  TraceOutputEnable,
  TraceMode0,
  TraceMode1,
//todo: check these against their apb bus and slot
  Timer2Stop=32,  //following word.
  Timer3Stop,
  Timer4Stop,
  Timer5Stop,
  Timer6Stop,
  Timer7Stop,
  Timer12Stop,
  Timer13Stop,
  Timer14Stop,
  skipanother,
  RtcStop,
  WindowWatchdogStop,
  WatchdogStop,

  I2c1TimeoutStop=32+21,
  I2c2TimeoutStop,
  I2c3TimeoutStop,
  yetanother,
  Can1Stop,
  Can2Stop,

  Timer1Stop=64,
  Timer8Stop,

  Timer9Stop=80,
  Timer10Stop,
  Timer11Stop,

};
#endif
inline void setDebugControl(enum DebugControlBit cbit){
  Ref<unsigned>(0xE0042004+cbit/32) |= 1 << (cbit%32);
}
//end of file.
