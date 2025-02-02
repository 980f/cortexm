
#if DEVICE==103
#include "flashf1.cpp"
#elif DEVICE==407 || DEVICE==411
#include "flashf4.cpp"
#elif DEVICE==452
#include "flashl4.cpp"
#endif
