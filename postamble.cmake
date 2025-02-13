# build standard outputs
# demand a custom linker script, standard ones are often not particularly standard:
set(LINKER_SCRIPT "${PROJECT_NAME}.ld")
#[[
The $project_name.ld file is expected to be a line to include the processor+vendor specific ld file and then the generic one:
INCLUDE cortexm/stm32/f411ce.ld
INCLUDE cortexm/cortexm.ld
]]

#todo: try to eliminate -specs=nosys.specs, we are suppressing routines that are referenced therein but will never execute.
set(CMAKE_EXE_LINKER_FLAGS "-L ${PROJECT_SOURCE_DIR} -T ${LINKER_SCRIPT} -nostartfiles  -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections,--print-memory-usage,-Map,${PROJECT_NAME}.map " CACHE INTERNAL "exe link flags")

set(SOURCES
  ${PROJECT_SPECIFIC_SOURCES}
  ${CORTEXUSAGES}
  ${${CortexmVendor}_VENDOR_USAGES}
  ${EZUSAGES}

)

add_executable(${PROJECT_NAME}.elf ${SOURCES})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

#make this conditional on file not existing, for now it is a manually issued command.
add_custom_command(TARGET ${PROJECT_NAME}.elf PRE_LINK
  COMMAND echo >../${LINKER_SCRIPT} "INCLUDE cortexm/${CortexmVendor}/${VendorPartname}${VendorVariant}.ld"
  COMMAND echo >>../${LINKER_SCRIPT} "INCLUDE cortexm/cortexm.ld"
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
  COMMAND ../cortexm/mapcleaner ${PROJECT_NAME}.map
  COMMENT "Building Intel hex file: ${HEX_FILE} "
  COMMAND ${OBJCOPY} -Oihex ${PROJECT_NAME}.elf ${HEX_FILE}
  COMMENT "Building binary image file:  ${BIN_FILE} "
  COMMAND ${OBJCOPY} -Obinary ${PROJECT_NAME}.elf ${BIN_FILE}
  COMMENT "Building DFU download file: ${PROJECT_NAME}.dfu "
  COMMAND ../cortexm/elf2dfuse ${PROJECT_NAME}.elf ${PROJECT_NAME}.dfu
)

add_custom_target(UPLOAD
  arm-none-eabi-gdb -iex "target remote tcp:127.0.0.1:3333"
  -iex "monitor program ${PROJECT_NAME}.elf"
  -iex "monitor reset init"
  -iex "disconnect" -iex "quit"
)
