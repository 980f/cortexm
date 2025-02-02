#pragma once

#if DEVICE==103
#include "gpiof1.h"
#elif DEVICE==407 || DEVICE==411
#include "gpiof4.h"
#elif DEVICE==452
#include "gpiol4.h"
#endif
