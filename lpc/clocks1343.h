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

#endif // CLOCKS_H
