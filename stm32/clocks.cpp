
#if DEVICE==103
#include "clocksf1.cpp"
#elif DEVICE==407 || DEVICE==411
#include "clocksf4.cpp"
#elif DEVICE==452
#include "clocksl4.cpp"
#endif
