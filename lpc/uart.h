#pragma once

#include "hook.h" //null checked handler functions
// interrupt id must be #defined, need it to make names.
#define uartIrq 46

namespace LPC {
/** uart manager class, not to be confused with hardware register class.
 * Note: if you pass the data source and sink in the constructor the instance can be const.
 */
  class Uart {
public:
    /** the isr. a stub is locally generated to vector to it */
    void isr(void)const;
private:
    /** called by isr on an input event.
     * negative values of @param are notifications of line errors, -1 for interrupts disabled */
    typedef Hooker<bool,~0,int /*incoming*/>::Pointer Receiver;
    Hooker<bool,~0,int /*incoming*/> receive;
    /** called by isr when transmission becomes possible.
     *  @return either an 8 bit unsigned character, or -1 to disable transmission events*/
    typedef Hooker<int,~0>::Pointer Sender;
    Hooker<int,~0> send;
protected:
    /** read at least one char from fifo, stop on error. @param LSRValue is recent read of line status register, @returns final read of line status register */
    unsigned tryInput(unsigned LSRValue)const;
public:
    //trying for full const'd instance
    Uart(Receiver receiver,Sender sender);

    /** set the baudrate using tweaky fields. @see baudsearch class for generating these value. */
    unsigned setBaudPieces(unsigned divider, unsigned mul, unsigned div, unsigned sysFreq) const;

    /** @param coded e.g. "8N1" for typical binary protocol, 8E2 for modbus */
    void setFraming(const char *coded) const;

    /** called to request that transmit isr be enabled or disabled.
     * Note: if enabled and wasn't enabled then send() will immediately be called to get first byte to send, IE send() isn't only called from isr (unless we can arrange for that!). */
    void beTransmitting(bool enabled = true)const;
    /** call to request that reception isr be enabled or disabled. */
    void reception(bool enabled = true)const;
    /** set send function. @returns this. */
    Uart &setTransmitter(Sender sender);
    /** set reception routine. . @returns this. */
    Uart &setReceiver(Receiver receiver);
    /** indirect access to the nvic interrupt enable */
    void irq(bool enabled)const;
    void initializeInternals()const;
    void setRxLevel(unsigned one48or14)const;
  };
} // namespace LPC


