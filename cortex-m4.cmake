#this file will get included from the part specific cmake file, such as stm32/f103.cmake, which is what you include in your top-level project

SET(CMAKE_SYSTEM_PROCESSOR Cortex-M4)
SET(gcccpu cortex-m4)  #todo: see if spelling of the above items matters to cmake, if not make it match gcc's case.

# CMSIS heritage defines:
ADD_DEFINITIONS(-D__CORTEX_M=4)
ADD_DEFINITIONS(-D__FPU_PRESENT=1)
ADD_DEFINITIONS(-D__MPU_PRESENT=1)
