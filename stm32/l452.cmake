
#processor specific but project independent parts of a CMake cortexM build.

#todo: what is the cmake pathing for included files? try to do this with a relative path.
#include("cortexm/cortexm4.cmake")
#the next are for some project generator module which I think I abandoned.
set(CHIP STM32L452)
set(CPU_FAMILY STM32L4xx)

#stack to CCM (or not) need to duplicate in linker control file at present.
ADD_DEFINITIONS(-DSTACKINCCM=0)
#vectors in ram gives a small improvement to interrupt overhead, but the CCM memory is not accessible to the NVIC so that ram cannot be used for it.
ADD_DEFINITIONS(-DVECTORSINRAM=0)

