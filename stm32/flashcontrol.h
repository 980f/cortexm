#pragma once

#if DEVICE==103
#include "flashf1.h"
#elif DEVICE==407 || DEVICE==411
#include "flashf4.h"
#else
#error "you must define DEVICE  to one of 103 | 407"
#endif /* ifndef flashControlH */
