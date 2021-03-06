#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "eztypes.h"

typedef u64 TimeValue;

/** an interval timer */
class StopWatchCore {

protected:
  TimeValue started;
  TimeValue stopped;
  bool running;
public:
  /** @param beRunning is whether to start timer upon construction.
      @param realElseProcess is whether to track realtime or thread-active-time */
  StopWatchCore(bool beRunning = true, bool realElseProcess = false);//defaults are best choice for function timing.

  /** use start and stop for non-periodic purposes*/
  void start();
  void stop();
  /** read elapsed time as ticks, for when you can't afford the do math in elapsed(), @param andRoll restarts timer when true */
  TimeValue peek(bool andRoll = false);
  /** convenient for passing around 'timeout pending' state */
  bool isRunning() const;
};

class StopWatch : public StopWatchCore {
public:
  using StopWatchCore::StopWatchCore;
  /** timestamp reported as seconds since roughly the start of the application */
  double asSeconds(const TimeValue);
  /** @returns elasped time and restarts interval. Use this for cyclic sampling. @param absolutely if not null gets the absolute time reading used in the returned value.*/
  double roll(double *absolutely = 0);
  /** updates 'stop' if running and @returns time between start and stop as seconds. @param absolutely if not null gets the absolute time reading used in the returned value.*/
  double elapsed(double *absolutely = 0);
  /** @return seconds of absolute time of stop, == now if running*/
  double absolute();

  /** @returns the number of cycles of frequency @param atHz that have @see elapsed() */
  unsigned cycles(double atHz, bool andRoll = true);

  /** @returns the number of cycles of @param ticks that have @see elapsed() */
  unsigned wraps(TimeValue ticks, bool andRoll = true);
};

class Timeout : public StopWatch {
public:
  TimeValue interval;
  using StopWatch::StopWatch;

  /** @returns whether timeout has expired, and if so either restarts it or stops it */
  bool check(bool andRestart);

  /** @returns whether timeout has expired, stops the timer if it has.  */
  bool timedout(bool whileRunning = true) {
    if(whileRunning){
      if(!isRunning()){
        return false;
      }
    }
    return check(false);
  }

  /** @returns whether timeout has expired, and if so restarts it  */
  bool cycled() {
    return check(true);
  }

  operator bool() const {
    return isRunning();
  }

//  void operator=(TimeValue timeout) {
//    interval = timeout;
//  }

  /** @returns how long until this guy timesout, negative if overdue which it will be if you don't check it often enough. */
  double dueIn();
};

#endif // STOPWATCH_H
