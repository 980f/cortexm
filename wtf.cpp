#include "wtf.h"
#include "cheaptricks.h"

extern "C" bool wtf(const int complaint) {
  static int lastWtf = 0;
  static int repeats = 0;
  if (int databreakpoint=complaint) {
    if (changed(lastWtf, databreakpoint)) {
      repeats = 0;
      return false;
    }
    ++repeats;
    return true;
  }
  return false;//wtf(0) does nothing at all
}
