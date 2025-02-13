#processor specific but project independent parts of a CMake cortexM build. included by cortexm/cortexm-all.cmake via defines.
set(LAST_IRQ 85)

#todo: create symbols for core and let -all include the following:
include("cortexm/cortex-m4.cmake")
##the next are for some project generator module which I think I abandoned.
#set(CHIP STM32F411)
#set(CPU_FAMILY STM32F40x)

#980f uses this for now, at least within stm32:
ADD_DEFINITIONS(-DDEVICE=411)

#nascent replacement for DEVICE usage, when whole file is swapped out or in via base named file which ifdef's on DEVICE.
set(GPIO_MODULE gpiof4)
set(CLOCK_MODULE clocksf4)

##[[
##Hal uses this:
#ADD_DEFINITIONS(-DSTM32F411xx)
#ADD_DEFINITIONS(-DDeviceHeaderFile="stm32f4xx.h")
##]]

#vectors in ram gives a small improvement to interrupt overhead, but the CCM memory is not accessible to the NVIC so that ram cannot be used for it.
ADD_DEFINITIONS(-DVECTORSINRAM=0)
