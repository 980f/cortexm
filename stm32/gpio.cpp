
#if DEVICE==103
#include "gpiof1.cpp"
#elif DEVICE==407 || DEVICE==411
#include "gpiof4.cpp"
#elif DEVICE==452
#include "gpiol4.cpp"
#endif
