#pragma once  // //(C) 2017,2018,2019 Andy Heilveil, github/980f

#if __has_include("Arduino.h")
#include "Arduino.h"
#endif

/** POSIX versions uses time_t classes which 980F/safely wraps, we mimic that here.
  Note: we use double but AVR uses 32bit for that. */
struct Microseconds {
  unsigned long micros;
  
  operator unsigned long()const {
    return micros;
  }

  operator double()const {
    return double(micros) / 1e6;
  }


  Microseconds& operator = (unsigned long ticks) {
    micros = ticks;
    return *this;
  }

  Microseconds &operator = (double seconds) {
    micros = seconds * 1e6; //truncating!
    return *this;
  }


  Microseconds& operator -=(const Microseconds &lesser) {
    micros -= lesser.micros;
    return *this;
  }

  Microseconds operator -(const Microseconds &lesser) const {
    Microseconds diff;
    diff = *this;
    diff -= lesser;
    return diff;
  }

  Microseconds& operator +=(const Microseconds &lesser) {
    micros += lesser.micros;
    return *this;
  }

  Microseconds operator +(const Microseconds &lesser) const {
    Microseconds diff;
    diff = *this;
    diff += lesser;
    return diff;
  }
  /* this implementation is optimized for returns of 0 and 1 and presumes non-negative this and positive interval */
  unsigned modulated(const Microseconds &interval) {
    if (interval.isZero()) {
      return 0;//gigo
    }
    unsigned cycles = 0;
    //we use repeated subtraction to do a divide since most times we cycle 0 or 1.
    while (*this >= interval) {
      *this -= interval;
      ++cycles;
    }
    return cycles;
  }
  
  bool operator >(const Microseconds &that) const {
    return micros > that.micros;
  }

  bool operator >=(const Microseconds &that) const {
    return micros > that.micros ;
  }
  
  bool operator <(const Microseconds &that) const {
    return micros < that.micros;
  }

  bool operator ==(const Microseconds &that) const {
    return micros == that.micros;
  }

  Microseconds &atLeast(const Microseconds &other)  {
    if (*this < other) {
      *this = other;
    } else if (isNever()) {
      *this = other;
    }
    return *this;
  }

  Microseconds &atMost(const Microseconds &other) {
    if (isNever()) {
      *this = other;
    } else if (*this > other) {
      *this = other;
    }
    return *this;
  }

  Microseconds &Never() {
    micros = ~0;
    return *this;
  }

  bool isNever()const {
    return micros == ~0U;
  }

  bool isZero() const noexcept {
    return micros == 0;
  }

#ifdef ARDUINO
  /** differs from posix due to implementation of delay being non-interruptible */
  void sleep()const {
    delayMicroseconds(micros);
  }

  /** arduino specific call to time since last program start. */
  Microseconds &snap(){
    *this= ::micros();
    return *this;
  }
#endif

};
