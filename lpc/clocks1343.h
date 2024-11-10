#ifndef CLOCKS_H
#define CLOCKS_H
#include "eztypes.h"

/** usually defined in a board file: */
extern const u32 EXTERNAL_HERTZ;

/** present values */
unsigned pllInputHz(bool forUsb);

/** @returns the frequency pre-divider */
unsigned coreInputHz();
/** @returns the approx frequency post-divider, rounded to nearest Hz
 * even if xtal is in integer Hz there is a divisor that isn't a power of 2 */
unsigned coreHz(void);

/** @returns Hz for @param bus number, -1 for systick, 0 for core, 1.. for AHB etc device specific values usually tied to apb index/*/
unsigned clockRate(int bus);

/** set clock to fastest possible with @internal osc else with its own logic as to which is faster.*/
void warp9(bool internal);

/* crafted for use with clock enable register these are also sometimes used as an arbitrary enum */
enum CK {
    SYS,ROM,RAM, //don't touch these! processor likely to need a power cycle
    FLASHREG,FLASHARRAY, //only touch these when you are fully running in ram (including vector table)
    I2C,
    GPIO,
    CT16B0,CT16B1,
    CT32B0,CT32B1,
    SSP,   //ssp0
    UART,
    ADC,
    USBREGulator,
    WDT,
    IOCON,
    reservedClockBit,
    SSP1,
};

#endif // CLOCKS_H
