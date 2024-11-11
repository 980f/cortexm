#processor specific but project independent parts of a CMake cortexM build.
set(LAST_IRQ 58)

include("cortexm/cortexm3.cmake")
#the next are for some project generator module which I think I abandoned.
set(CHIP STM32F103)
set(CPU_FAMILY STM32F10x)

#980f uses this for now:
ADD_DEFINITIONS(-DDEVICE=103 -DuseSTM32)
INCLUDE_DIRECTORIES("cortexm/stm32")
#nascent replacement for DEVICE usage, when whole file is swapped out or in.
set(GPIO_MODULE gpiof1)
set(CLOCK_MODULE clocksf1)

#[[
#Hal uses this:
ADD_DEFINITIONS(-DSTM32F103xx)
ADD_DEFINITIONS(-DDeviceHeaderFile="stm32f1xx.h")
]]

#stack to CCM (or not) need to duplicate in linker control file at present. (and this probably should be in a project file, we'll see if CMAKE allows overrides when coalescing -D's)
ADD_DEFINITIONS(-DSTACKINCCM=0)
#vectors in ram gives a small improvement to interrupt overhead, but the CCM memory is not accessible to the NVIC so that ram cannot be used for it.
ADD_DEFINITIONS(-DVECTORSINRAM=0)

