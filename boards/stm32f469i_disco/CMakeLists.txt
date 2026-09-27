set(STM32CUBE_F4_ROOT "" CACHE PATH "Root of STM32Cube_FW_F4 package")
if(NOT EXISTS "${STM32CUBE_F4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h")
    message(FATAL_ERROR "Set STM32CUBE_F4_ROOT to an STM32Cube_FW_F4 package containing the HAL and STM32469I-Discovery BSP")
endif()

set(cube "${STM32CUBE_F4_ROOT}")
option(F469_DISCO_REVA "Use the early Discovery Rev A 25 MHz oscillator and DSI setup" OFF)
set(hal "${cube}/Drivers/STM32F4xx_HAL_Driver")
set(core "${CMAKE_CURRENT_SOURCE_DIR}/Core")
set(bsp "${cube}/Drivers/BSP/STM32469I-Discovery")
set(components "${cube}/Drivers/BSP/Components")
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl/src/*.c")

add_executable(f469_ui
    ${core}/startup_stm32f469nihx.s
    ${core}/system_stm32f4xx.c
    ${core}/main.c
    ${core}/clock.c
    ports/display_touch.c
    ports/f469_touch_logic.c
    ports/msp_uart_f469.c
    ports/lamp_f469.c
    ports/ws2812_f469.c
    ${hal}/Src/stm32f4xx_hal.c
    ${hal}/Src/stm32f4xx_hal_cortex.c
    ${hal}/Src/stm32f4xx_hal_flash.c
    ${hal}/Src/stm32f4xx_hal_flash_ex.c
    ${hal}/Src/stm32f4xx_hal_gpio.c
    ${hal}/Src/stm32f4xx_hal_pwr.c
    ${hal}/Src/stm32f4xx_hal_pwr_ex.c
    ${hal}/Src/stm32f4xx_hal_rcc.c
    ${hal}/Src/stm32f4xx_hal_rcc_ex.c
    ${hal}/Src/stm32f4xx_hal_dma.c
    ${hal}/Src/stm32f4xx_hal_dma2d.c
    ${hal}/Src/stm32f4xx_hal_dsi.c
    ${hal}/Src/stm32f4xx_ll_fmc.c
    ${hal}/Src/stm32f4xx_hal_i2c.c
    ${hal}/Src/stm32f4xx_hal_i2c_ex.c
    ${hal}/Src/stm32f4xx_hal_ltdc.c
    ${hal}/Src/stm32f4xx_hal_ltdc_ex.c
    ${hal}/Src/stm32f4xx_hal_sdram.c
    ${hal}/Src/stm32f4xx_hal_uart.c
    ${hal}/Src/stm32f4xx_hal_tim.c
    ${hal}/Src/stm32f4xx_hal_tim_ex.c
    ${hal}/Src/stm32f4xx_hal_iwdg.c
    ${bsp}/stm32469i_discovery.c
    ${bsp}/stm32469i_discovery_lcd.c
    ${bsp}/stm32469i_discovery_sdram.c
    ${bsp}/stm32469i_discovery_ts.c
    ${components}/otm8009a/otm8009a.c
    ${components}/nt35510/nt35510.c
    ${components}/ft6x06/ft6x06.c
    ${LVGL_SOURCES}
    ${CMAKE_SOURCE_DIR}/App/Src/ui/f469_ui.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/f469_face.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/f469_info.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/ui_theme.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/face_model.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/two_screen_controller.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/overview_model.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/low_battery_policy.c
    ${CMAKE_SOURCE_DIR}/App/Src/msp/msp_codec.c
    ${CMAKE_SOURCE_DIR}/App/Src/msp/msp_client.c
    ${CMAKE_SOURCE_DIR}/App/Src/app_time.c
    ${CMAKE_SOURCE_DIR}/App/Src/vehicle_state.c
    ${CMAKE_SOURCE_DIR}/App/Src/vehicle_state_source.c
    ${CMAKE_SOURCE_DIR}/App/Src/demo_vehicle_state.c
    ${CMAKE_SOURCE_DIR}/App/Src/lighting/lighting_controller.c
    ${CMAKE_SOURCE_DIR}/App/Src/lighting/f469_vehicle_effects.c
    ${CMAKE_SOURCE_DIR}/App/Src/lighting/lighting_service.c
    ${CMAKE_SOURCE_DIR}/App/Src/led/led_controller.c
)
target_include_directories(f469_ui PRIVATE
    ${core}
    ${hal}/Inc
    ${cube}/Drivers/CMSIS/Include
    ${cube}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${bsp}
    ${components}/Common
    ${components}/ft6x06
    ${cube}/Utilities/Fonts
    ${CMAKE_CURRENT_SOURCE_DIR}/ports
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/App/Inc
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl
)
target_compile_definitions(f469_ui PRIVATE STM32F469xx USE_HAL_DRIVER LV_CONF_INCLUDE_SIMPLE F469_RC_LAYOUT=1)
if(F469_DISCO_REVA)
    target_compile_definitions(f469_ui PRIVATE USE_STM32469I_DISCO_REVA)
else()
    target_compile_definitions(f469_ui PRIVATE USE_STM32469I_DISCO_REVB)
endif()
target_compile_options(f469_ui PRIVATE
    -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    -ffunction-sections -fdata-sections -Wall -Wextra
)
target_link_options(f469_ui PRIVATE
    -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F469NIHX_FLASH.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/f469_ui.map
)
set_target_properties(f469_ui PROPERTIES SUFFIX ".elf")
target_link_libraries(f469_ui PRIVATE m)
add_custom_command(TARGET f469_ui POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:f469_ui>
            ${CMAKE_CURRENT_BINARY_DIR}/f469_ui.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:f469_ui>
            ${CMAKE_CURRENT_BINARY_DIR}/f469_ui.hex
    VERBATIM)
