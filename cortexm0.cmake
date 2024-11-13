SET(CMAKE_SYSTEM_PROCESSOR Cortex-M0)
SET(gcccpu cortex-m0plus)

# CMSIS heritage defines:
ADD_DEFINITIONS(-D__CORTEX_M=0)
ADD_DEFINITIONS(-D__FPU_PRESENT=0)
ADD_DEFINITIONS(-D__MPU_PRESENT=0)

include (cortexm/cortexm-all.cmake)