#ifndef FIFO_H
#define FIFO_H

/** a fifo suitable for one way data flow between routines running at different nvic priorities.
 * It only guards against read vs write conflicts, it does not deal with attempts to read nor write from more than one thread.
 */
class Fifo {
  unsigned count;
  unsigned char *reader;
  unsigned char *writer;
  /** the memory*/
  unsigned char * const mem;
  unsigned char * const end;
  const unsigned quantity;

  /** circularly increment reader or writer */
  inline void incrementPointer(unsigned char *&pointer){
    if(++pointer >= end) {//'>' is a COA while we're hunting for the fifo read error bug.
      pointer= mem;
    }
  }

  /** copying doesn't make sense*/
  Fifo(const Fifo&)=delete;
public:
  Fifo(unsigned quantity,unsigned char *mem);

  /** forget the content */
  void clear();

  /** forget the content AND wipe the memory, the latter useful for debug.*/
  void wipe();

  /** @returns bytes present, but there may be more or less real soon. */
  inline unsigned available() const {
    return count;
  }

  /** @returns bytes empty, but there may be more or less real soon. */
  inline unsigned free() const {
    return quantity-count;
  }

  /** tries to put a byte into the memory, @returns whether there was room*/
  bool insert(unsigned char incoming);

  /** try to insert a byte, @returns whether full (-1), busy (-2), or succeeded (0).*/
  int attempt_insert(unsigned char incoming);

  /** reads and removes a byte from the memory, @returns the byte, or -1 if there wasn't one*/
  int remove();

  /** tries to insert a byte, @returns whether empty (-1), busy (-2), or succeeded (char removed).*/
  int attempt_remove();

   /** @returns how many did NOT get pushed */
  unsigned stuff(const char *block,unsigned length);

  /** @returns 0 for in bounds, 1 or -1 for outside of bounds.*/
  int boundsError(bool reads)const;
};

/** allocate data and wrap it in a Fifo access mechanism.
 * Added some syntactic sugar, insert and extract via assignment and cast  */
template <int size> class FifoBuffer:public Fifo {
public:
  unsigned char buf[size];
  FifoBuffer():Fifo(sizeof(buf),buf){}
  bool operator =(unsigned char received){
    return insert(received);
  }
  /** @returns char removed from fifo, or negative number for various errors. */
  operator int(){
    return remove();
  }

};


#endif // FIFO_H
