set(BOOT_STUB_TARGET wio_ai_boot_stub)
set(BOOT_STUB_LINKER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/STM32H725AEIX_BOOT.ld")

add_executable(${BOOT_STUB_TARGET} boot_stub.S)
set_property(TARGET ${BOOT_STUB_TARGET} PROPERTY LINKER_LANGUAGE C)
set_property(TARGET ${BOOT_STUB_TARGET} APPEND PROPERTY
    LINK_DEPENDS "${BOOT_STUB_LINKER_SCRIPT}")

target_compile_options(${BOOT_STUB_TARGET} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -ffunction-sections
    -fdata-sections
)
target_link_options(${BOOT_STUB_TARGET} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -nostdlib
    -Wl,--gc-sections
    -Wl,--build-id=none
    "-T${BOOT_STUB_LINKER_SCRIPT}"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${BOOT_STUB_TARGET}.map"
)

add_custom_command(TARGET ${BOOT_STUB_TARGET} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${BOOT_STUB_TARGET}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${BOOT_STUB_TARGET}>
            "${CMAKE_CURRENT_BINARY_DIR}/${BOOT_STUB_TARGET}.hex"
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${BOOT_STUB_TARGET}>
            "${CMAKE_CURRENT_BINARY_DIR}/${BOOT_STUB_TARGET}.bin"
)
