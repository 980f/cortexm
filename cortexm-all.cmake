#[[processor specific but project independent parts of a CMake cortexM build.
include this file in the processor specific cmake file and include that in your project specific cmakelist.txt
see also postable.cmake, this guy sets that guy up but you have to do some defining in between this file and that one.
also sometimes you need to run just this file to get the compiler settings cached.
]]

#kill a warning: qt sets this when it launches cmake, but we don't produce QT stuff so we got a warning.
UNSET(QT_QMAKE_EXECUTABLE)

# Magic settings. Without it CMake tries to run test programs on the host platform, which fails of course.
SET(CMAKE_SYSTEM_NAME Generic)
SET(TARGET arm-arm-none-eabi)

#we use a cross compiler, your OS *might* have a usable one, but I downloaded one from developer.arm.com:
SET(BINROOT /r/bin/gcc-arm-none-eabi/bin/)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# specify the cross compiler but don't let cmake test it, it tries to run a program on the host which doesn't work for microcontroller builds
SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_C_COMPILER ${BINROOT}arm-none-eabi-gcc)
SET(CMAKE_CXX_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER ${BINROOT}arm-none-eabi-g++)

SET(CMAKE_ASM_COMPILER_WORKS 1)
SET(CMAKE_ASM_COMPILER ${BINROOT}arm-none-eabi-as)

SET(AS ${BINROOT}arm-none-eabi-as)
SET(AR ${BINROOT}arm-none-eabi-ar)
SET(OBJCOPY ${BINROOT}arm-none-eabi-objcopy)
SET(OBJDUMP ${BINROOT}arm-none-eabi-objdump)
SET(SIZE ${BINROOT}arm-none-eabi-size)

#had to add this to get cortexm3.s to be processed:
ENABLE_LANGUAGE(ASM)
#wildly guessing on trying to get correct asm language:
SET(CMAKE_ASM_COMPILER_TARGET ${gcccpu})

#cli stuff common to assembler and compiler:
SET(gcc_arch "-mcpu=${gcccpu} -mfloat-abi=soft ")

SET(CMAKE_ASM_FLAGS " ${gcc_arch} ")

#stuff common to C and C++. the -sections allows the linker to finely prune the output.
SET(ccli_common " ${gcc_arch}  -fdata-sections -ffunction-sections -Wall -ffreestanding")

#3rd party code such as STM's HAL does extensive type punning of integers to addresses:
SET(CMAKE_C_FLAGS "${ccli_common} -std=c11 -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast " CACHE INTERNAL "c compiler flags")

# -Wno-unknown-pragmas added to hide spew from clang pragmas that hide clang spew. phew!
SET(CMAKE_CXX_FLAGS "${ccli_common}  -Wno-unknown-pragmas -fno-rtti -fno-exceptions -fno-threadsafe-statics -MD " CACHE INTERNAL "cxx compiler flags")

# every project needs cstartup, and we wish to optimize it independent of whether the rest of the project gets optimized
#the below does not work
set(CORTEXUSAGES  ${CORTEXUSAGES}
  "cortexm/cstartup.cpp"
)

set_source_files_properties(cortexm/cstartup.cpp PROPERTIES COMPILE_OPTIONS "-g0;-O2;-fomit-frame-pointer")

# build the vector table file, set LAST_IRQ in your processor definition cmake file. You may have to manually delete this file when building for a different chip.
ADD_CUSTOM_COMMAND(
  OUTPUT ${PROJECT_SOURCE_DIR}/nvicTable.inc
  COMMAND cortexm/mkIrqs ${LAST_IRQ} >nvicTable.inc
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  DEPENDS ../cortexm/mkIrqs ${CMAKE_CURRENT_LIST_FILE}
)


# build the vector table file, set LAST_IRQ in your processor definition cmake file.
#can't figure out how to invoke this function, until then will continue to use a bash script mkIrqs.
#FUNCTION(GENIRQTABLE lastirq)
#  SET(linkfile "//Table for $lastirq interrupts")
#  FOREACH (anirq RANGE ${lastirq})
#    LIST(APPEND linkfile "stub( ${anirq} )\;\n")
#  ENDFOREACH ()
#  LIST(APPEND linkfile "Handler VectorTable[] __attribute__((section(\".vectors.3\"))) = {")
#  FOREACH (anirq RANGE ${lastirq})
#    LIST(APPEND linkfile "" IrqName (${anirq}) ,"\n")
#  ENDFOREACH ()
#  LIST(APPEND linkfile  "};")
#  MESSAGE($(linkfile))
#ENDFUNCTION()

