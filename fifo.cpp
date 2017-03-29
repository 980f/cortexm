#include "fifo.h"

#include "core-atomic.h" // so that routines at different interrupt priorities can talk to each other using a fifo.

Fifo::Fifo(unsigned quantity, unsigned char *mem): mem(mem), end(mem + quantity), quantity(quantity){
  clear();
}

void Fifo::clear(){
  reader = writer = mem;
  count = 0;
}

void Fifo::wipe(){
  clear();
  for(unsigned i = count; i-- > 0; ) {
    mem[i] = 0;
  }
}

/** the following only accomodates a single writer thread, the atomicity only deals with read vs write. */
int Fifo::attempt_insert(unsigned char incoming){
  if(count < quantity) {
    *writer = incoming;
    if(atomic_increment(count)) {
      return -2;
    }
    incrementPointer(writer);
    return 0;
  } else {
    return -1;
  }
} // Fifo::attempt_insert

bool Fifo::insert(unsigned char incoming){
  int pushed;
  do {
    pushed = attempt_insert(incoming);
  } while(pushed == -2);
  return pushed ? false : true;
}

/** the following only accomodates a single reader thread, the atomicity only deals with read vs write. */
int Fifo::attempt_remove(){
  if(count > 0) { // can't alter count until we have preserved our datum, so no test_and-decrement stuff.
    unsigned pulled = *reader;
    // alter count before pointer to reduce the window for collision. (if we bail on collisions, else is moot)
    if(atomic_decrement(count)) {
      return -2;
    }
    incrementPointer(reader);
    return int(pulled);//chars 128->255 are positive
  } else {
    return -1;
  }
} // Fifo::attempt_remove

int Fifo::remove(){
  int pulled;
  do {
    pulled = attempt_remove();
  } while(pulled == -2);
  return pulled;
}

 /** @returns how many did NOT get pushed */
unsigned Fifo::stuff(const char *block,unsigned length){
  while(length-->0){
    if(insert(*block++)<0){
      return ++length;
    }
  }
  return 0;
}

int Fifo::boundsError(bool reads) const {
  if(reads){
    if(reader>=end){
      return 1;
    }
    if(reader<mem){
      return -1;
    }
  } else {//## leave this implementation expanded, so we can breakpoint on each independent issue
    if(writer>=end){
      return 1;
    }
    if(writer<mem){
      return -1;
    }
  }
  return 0;//strange, compiler didn't mention this missing return 
}
