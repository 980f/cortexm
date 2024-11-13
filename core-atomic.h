#ifndef COREATOMIC_H
#define COREATOMIC_H

/** @return whether the alignedDatum FAILED to increment.
 * The most common reason for a failure would be that an interrupt occurred during the operation.
If you are sure the cause of failure isn't permanent then: do{}while(atomic_increment(arg));
*/
bool atomic_increment(unsigned &alignedDatum);

/** @return whether the alignedDatum FAILED to decrement */
bool atomic_decrement(unsigned &alignedDatum);

/** @return whether the following logic succeeded, not whether it actually decremented: if datum is not zero decrement it */
bool atomic_decrementNotZero(unsigned &alignedDatum);

/** @return whether the following logic succeeded, not whether it actually incremented: if datum is not all ones then increment it */
bool atomic_incrementNotMax(unsigned &alignedDatum);

/** @returns 1 if the value was zero in which case it is still 0, else blocks until decrement and returns 0 (regardless of decremented value) */
bool atomic_decrementNowZero(unsigned &alignedDatum);

/** @returns whether the value was zero before increment, if datum is all ones then left all ones. */
bool atomic_incrementWasZero(unsigned &alignedDatum);

/** @return whether the following logic succeeded, if datum IS zero then replace it with given value */
bool atomic_setIfZero(unsigned &alignedDatum,unsigned value);

#endif // COREATOMIC_H
