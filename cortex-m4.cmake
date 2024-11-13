SET(CMAKE_SYSTEM_PROCESSOR Cortex-M4)
SET(gcccpu cortex-m4)  #todo: see if spelling of the above items matters to cmake, if not make it match gcc's case.

# CMSIS heritage defines:
ADD_DEFINITIONS(-D__CORTEX_M=4)
ADD_DEFINITIONS(-D__FPU_PRESENT=1)
ADD_DEFINITIONS(-D__MPU_PRESENT=1)

include (cortexm/cortexm-all.cmake)
