#processor specific but project independent parts of a CMake cortexM build.
set(LAST_IRQ 81)

include("cortexm/cortexm4.cmake")
#the next are for some project generator module which I think I abandoned.
set(CHIP STM32F407)
set(CPU_FAMILY STM32F40x)

#980f uses this for now:
ADD_DEFINITIONS(-DDEVICE=407)
#nascent replacement for DEVICE usage, when whole file is swapped out or in.
set(GPIO_MODULE gpiof4)
set(CLOCK_MODULE clocksf4)

#[[
#Hal uses this:
ADD_DEFINITIONS(-DSTM32F407xx)
ADD_DEFINITIONS(-DDeviceHeaderFile="stm32f4xx.h")
]]

#stack to CCM (or not) need to duplicate in linker control file at present. (and this probably should be in a project file, we'll see if CMAKE allows overrides when coalescing -D's)
ADD_DEFINITIONS(-DSTACKINCCM=1)
#vectors in ram gives a small improvement to interrupt overhead, but the CCM memory is not accessible to the NVIC so that ram cannot be used for it.
ADD_DEFINITIONS(-DVECTORSINRAM=0)

