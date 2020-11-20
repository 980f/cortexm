#processor specific but project independent parts of a CMake cortexM build.


#
set(LAST_IRQ 84)
include(cortexm/cortexm4.cmake)


#the next are for some project generator module which I think I abandoned.
set(CHIP STM32L452)
set(CPU_FAMILY STM32L4xx)
add_definitions(-DDEVICE=452)

#set(LAST_IRQ 84)


#stack to CCM (or not) need to duplicate in linker control file at present to make it happen.
ADD_DEFINITIONS(-DSTACKINCCM=1)
#vectors in ram gives a small improvement to interrupt overhead, but the CCM memory is not accessible to the NVIC so that ram cannot be used for it.
ADD_DEFINITIONS(-DVECTORSINRAM=0)
