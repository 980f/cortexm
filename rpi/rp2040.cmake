#processor specific but project independent parts of a CMake cortexM build.
set(LAST_IRQ 16)

include("cortexm/cortexm0.cmake")
#the next are for some project generator module which I think I abandoned.
set(CHIP RP2040)
set(CPU_FAMILY RP)

ADD_DEFINITIONS(-DDEVICE=2040)


