#processor specific but project independent parts of a CMake cortexM build.
#what is the highest numbered interrupt:
set(LAST_IRQ 84)

#the next are for some project generator module which I think I abandoned.
set(CHIP STM32L452)
set(CPU_FAMILY STM32L4xx)

#980f stuff uses this for model specific differences to stm peripherals
add_definitions(-DDEVICE=452)

set(GPIO_MODULE gpiol4)
set(CLOCK_MODULE clocksl4)

#stack to CCM (or not) need to duplicate in linker control file at present to make it happen.
ADD_DEFINITIONS(-DSTACKINCCM=1)
#vectors in ram gives a small improvement to interrupt overhead, but the CCM memory is not accessible to the NVIC so that ram cannot be used for it.
ADD_DEFINITIONS(-DVECTORSINRAM=0)

#include the processor's core support:
include(cortexm/cortexm4.cmake)