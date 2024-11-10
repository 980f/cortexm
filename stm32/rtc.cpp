//not compiling, very stale.
#include "rtc.h"

#include "nvic.h"

ObjectInterrupt(theRTC->isr(), 41)

enum regId { pre = 0, div, now, alarm };

u16 *RTC::regPointer(int regid){
  u16 *base = reinterpret_cast <u16 *> (registerAddress());

  return &base[6 + 2 * regid];
}

u32 RTC::reg(int regid){
  u16 *reg = regPointer(regid);
  u16 high = reg[-2];
  u16 low = reg[0];

  return high << 16 | low;
}

void RTC::setreg(int regid, u32 value){
  u16 *reg = regPointer(regid);

  reg[-2] = value >> 16;
  reg[0] = value;
}

void RTC::isr(void){
  utcSeconds = reg(now);
  band[3] = 0; //
}


void RTC::init(void){
  APBdevice::init();
  //pwren  bkpen
  //dbp
  band[0] = 1; //one second interrupt enable.
}

void RTC::configure(u32 crystalHz){
  //todo: make LSE run
  //must use LSE to get battery backing
  //todo: set conf for LSE
  //calibrate LSE? --no, we aren't using an external HS clock so we have no basis for calibration.
  //set divisor for 1 second
  setreg(pre, crystalHz - 1);
  //
}
