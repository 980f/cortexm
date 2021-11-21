#processor specific but project independent parts of a CMake cortexM build.

#must set LAST_IRQ before including cortexm..cmake else irq table maker throws errors
set(LAST_IRQ 25)

include("cortexm/cortexm0.cmake")

#the following are for files that are sensitive to the exact model of a family of chips

ADD_DEFINITIONS(-DDEVICE=2040)

#the next are for some project generator module which I think I abandoned.
set(CHIP RP2040)
set(CPU_FAMILY RP)



