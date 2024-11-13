#processor specific but project independent parts of a CMake cortexM build.
set(LAST_IRQ 58)

include("cortexm/cortex-m3.cmake")
#the next are for some project generator module which I think I abandoned.
set(CHIP STM32F103)
set(CPU_FAMILY STM32F10x)

#980f uses this for now:
ADD_DEFINITIONS(-DDEVICE=103 -DuseSTM32)

#nascent replacement for DEVICE usage, when whole file is swapped out or in.
set(GPIO_MODULE gpiof1)
set(CLOCK_MODULE clocksf1)

#[[
#Hal uses this:
ADD_DEFINITIONS(-DSTM32F103xx)
ADD_DEFINITIONS(-DDeviceHeaderFile="stm32f1xx.h")
]]

ADD_DEFINITIONS(-DVECTORSINRAM=0)
