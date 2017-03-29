#pragma once

//replaced with inheritance, same #bytes #include "hook.h" //null checked handler functions
// interrupt id must be #defined, need it to make names.
#define uartIrq 46

namespace LPC {
/** uart manager class, not to be confused with hardware register class.
 * Note: if you pass the data source and sink in the constructor the instance can be const.
 */
class Uart {
public: //configuration
  /** set the baudrate using tweaky fields. @see baudsearch class for generating these value. */
  unsigned setBaudPieces(unsigned divider, unsigned mul, unsigned div, unsigned sysFreq) const;

  /**a 3 bit field with 5 usable settings:  Force:LowActive:Enable */
  enum Parity {
    NoParity=0,
    OddParity=1,
    EvenParity=3,
    MarkParity=5,
    SpaceParity=7
  };
  /** @param coded e.g. "8N1" for typical binary protocol, 8E2 for modbus */
  void setFraming(unsigned numbits, Parity parityCode, unsigned stops) const;

  /** called to request that transmit isr be enabled or disabled.
     * Note: if enabled and wasn't enabled then send() will immediately be called to get first byte to send, IE send() isn't only called from isr (unless we can arrange for that!). */
  bool beTransmitting(bool enabled = true) const;
  /** call to request that reception isr be enabled or disabled. */
  void reception(bool enabled = true) const;

  /** indirect access to the nvic interrupt enable */
  void irq(bool enabled) const;
  void initializeInternals() const;
  void setRxLevel(unsigned one48or14) const;
  void setLoopback(bool on) const;

};

/** at one time this was gratuitously const'd. There is no data members so constness doesn't gain much */
class UartHandler : public Uart {
public:
  /** the isr. you will have to write a stub to vector to it. For the lpc13xx there can be only one uart, but the 43xx family has multiple so we use an othewise pointless class here in the hopes it will be useful later. */
  void isr(void);

protected:
  /** called by isr on an input event.
       * negative values of @param are notifications of line errors, @return false to disable reception.
       * ls byte of negative value is datum causing error, high byte is ~LSR. */
  virtual bool receive(int incoming);
  /** called by isr when transmission becomes possible.
       *  @return either an 8 bit unsigned character, or -1 to disable transmission events*/
  virtual int send();

public: //for polling and prodding
  /** read at least one char from fifo, stop on error. @returns whatver receive hook wants to do about the datum */
  void tryInput();
  void stuffsome();
};
} // namespace LPC
