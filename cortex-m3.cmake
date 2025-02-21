SET(CMAKE_SYSTEM_PROCESSOR Cortex-M3)
SET(gcccpu cortex-m3)  #todo: see if spelling of the above items matters to cmake, if not make it match gcc's case.

message("In file cortex-m3.cmake")

# CMSIS heritage defines:
ADD_DEFINITIONS(-D__CORTEX_M=3)
ADD_DEFINITIONS(-D__FPU_PRESENT=0)
ADD_DEFINITIONS(-D__MPU_PRESENT=0)

#softfp uses hard fp parameter passing.
set(FP_API "softfp")

set(GLOBAL CORTEXMUSAGES
  "cortexm/cortexm3.cpp"
)


include (cortexm/cortexm-all.cmake)
