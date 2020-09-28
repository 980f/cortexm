#include "stopwatch.h"

#include "cheaptricks.h"  //flagged

#ifdef ARDUINO

extern TimeValue millis();
void readit(TimeValue &ts){
  ts=millis();
}

double StopWatch::asSeconds(const TimeValue ts){
  return (ts)*1000;//arduino hardcoded to millisecond.
}

#elif defined(__linux__)
void readit(TimeValue &){
//  ts=snapLongTime();
}

double StopWatch::asSeconds(const TimeValue ){
//  return secondsForLongTime(ts);
}

#else
#include "systick.h"
using namespace SystemTimer;

void readit(TimeValue &ts){
  ts=snapLongTime();
}

double StopWatch::asSeconds(const TimeValue ts){
  return secondsForLongTime(ts);
}

#endif

StopWatchCore::StopWatchCore(bool beRunning,bool /*realElseProcess*/){
  readit(started);
  stopped = started;
  running = beRunning;
}

void StopWatchCore::start(){
  readit(started);
  running = true;
}

void StopWatchCore::stop(){
  if(flagged(running)) {
    readit(stopped);
  }
}

TimeValue StopWatchCore::peek(bool andRoll){
  if(running){
    readit(stopped);
  }
  TimeValue elapsed=started-stopped;

  if(andRoll){
    started=stopped;
  }
  return elapsed;
}

bool StopWatchCore::isRunning() const {
  return running;
}

/////////////////////
double StopWatch::absolute(){
  if(running) {
    readit(stopped);
  }
  return asSeconds(stopped);
}

double StopWatch::elapsed(double *absolutely){
  TimeValue delta=peek(false);
  if(absolutely) {
    *absolutely = asSeconds(stopped);
  }
  //todo: if delta<0 ....
  return asSeconds(delta);
} // StopWatch::elapsed


double StopWatch::roll(double *absolutely){
  TimeValue delta=peek(true);
  if(absolutely) {
    *absolutely = asSeconds(stopped);
  }
  return asSeconds(delta);
}

unsigned StopWatch::cycles(double atHz,bool andRoll){
  double seconds=elapsed();
  //could provide for more precise cycling by adjusting start by fraction of events *1/atHz.
  double events=seconds*atHz;
  auto cycles=unsigned(events);//#we want truncation, rounding would be bad.
  if(andRoll){
    if(cycles>0){//only roll if the caller is going to take action.
      if(running) {
        started = stopped;//#do NOT start(), want to read the clock just once with each roll.
        // a refined version would subtract out the fractional part of events/atHz, for less jitter.
        //if we did that we could drop the test for cycles>0, however doing that test is efficient.
      }
    }
  }
  return cycles;
}

