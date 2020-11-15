#processor specific but project independent parts of a CMake cortexM build.
#include this file in the processor specific cmake file and include that in your project specific cmakelist.txt
#see also postable.cmake, this guy sets that guy up but you have to do some defining in between this file and that one.
#also sometimes you need to run just this file to get the compiler settings cached.

#kill warning: qt sets this when it launches cmake, but we don't produce QT stuff so we got a warning.
unset(QT_QMAKE_EXECUTABLE)

# Magic settings. Without it CMake tries to run test programs on the host platform, which fails of course.
SET (CMAKE_SYSTEM_NAME Generic)


set(CMAKE_SYSTEM_PROCESSOR Cortex-M4)
set(TARGET arm-arm-none-eabi)

#we use a cross compiler, your OS *might* have a usable one, but I downloaded one from developer.arm.com:
SET(BINROOT /d/bin/gcc-arm-none-eabi/bin/)

# specify the cross compiler but don't let cmake test it, it tries to run a program on the host which doesn't work for microcontroller builds
SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_C_COMPILER ${BINROOT}arm-none-eabi-gcc)
SET(CMAKE_CXX_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER ${BINROOT}arm-none-eabi-g++)
set(AS ${BINROOT}arm-none-eabi-as)
set(AR ${BINROOT}arm-none-eabi-ar)
set(OBJCOPY ${BINROOT}arm-none-eabi-objcopy)
set(OBJDUMP ${BINROOT}arm-none-eabi-objdump)
set(SIZE ${BINROOT}arm-none-eabi-size)


#had to add this to get cortexm3.s to be processed:
enable_language(ASM)
#wildy guessing on trying to get correct asm language:
SET(CMAKE_ASM_COMPILER_TARGET cortex-m4)
SET(CMAKE_ASM_FLAGS " -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 ")

#C and C++ do have a lot of commonality:
SET(ccli_common " -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fdata-sections -ffunction-sections -Wall ")

#3rd party code such as STM's HAL does extensive type punning of integers to addresses:
SET(CMAKE_C_FLAGS "${ccli_common} -std=c11 -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast " CACHE INTERNAL "c compiler flags")

# -Wno-unknown-pragmas added to hide spew from clang pragmas that hide clang spew. phew!
SET(CMAKE_CXX_FLAGS "${ccli_common}  -std=c++17  -Wno-unknown-pragmas -fno-rtti -fno-exceptions -MD " CACHE INTERNAL "cxx compiler flags")



