#ifndef SHADOW_H
#define SHADOW_H

#include "eztypes.h"
/**
hardware registers that don't tolerate byte writes can't be directly accessed via C bit fields.
The below only fully works on registers none of whose bits change spontaneously (no status bits, or read-only).
Use bitband bits as much as possible instead of this.

This class will do bizarre things if Fielder type has any virtual components.
*/

template <typename Fielder,typename U>
class Shadowed: public Fielder {
  /** integer wide enough to cover an instance of Fielder */
  volatile U &raw;
public:
  Shadowed(Address address):raw(* reinterpret_cast<volatile U*>(address)){
    pun(U,*this)=raw;
  }
  void update(void){
    raw=pun(U,*this);
  }
};

#endif // SHADOW_H
