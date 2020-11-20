# build standard outputs
# demand a custom linker script, standard ones are often not particularly standard:
SET(LINKER_SCRIPT "${PROJECT_NAME}.ld")

#todo: try to eliminate -specs=nosys.specs, we are suppressing routines that are referenced therein but will never execute.
SET(CMAKE_EXE_LINKER_FLAGS "-L ${PROJECT_SOURCE_DIR} -T ${LINKER_SCRIPT} -nostartfiles  -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections,--print-memory-usage,-Map,${PROJECT_NAME}.map " CACHE INTERNAL "exe link flags")

ADD_EXECUTABLE(${PROJECT_NAME}.elf ${SOURCES})


SET(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
SET(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME}.elf POST_BUILD
  COMMAND ../cortexm/mapcleaner ${PROJECT_NAME}.map
  COMMENT "Building Intel hex file: ${HEX_FILE} "
  COMMAND ${OBJCOPY} -Oihex ${PROJECT_NAME}.elf ${HEX_FILE}
  COMMENT "Building binary image file:  ${BIN_FILE} "
  COMMAND ${OBJCOPY} -Obinary ${PROJECT_NAME}.elf ${BIN_FILE}
  COMMENT "Building DFU download file: ${PROJECT_NAME}.dfu "
  COMMAND ../cortexm/elf2dfuse ${PROJECT_NAME}.elf ${PROJECT_NAME}.dfu
  )

ADD_CUSTOM_TARGET(UPLOAD
  arm-none-eabi-gdb -iex "target remote tcp:127.0.0.1:3333"
  -iex "monitor program ${PROJECT_NAME}.elf"
  -iex "monitor reset init"
  -iex "disconnect" -iex "quit"
  )
