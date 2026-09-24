set(STM32CUBE_F4_ROOT "" CACHE PATH "Root of STM32Cube_FW_F4 package")
if(NOT EXISTS "${STM32CUBE_F4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h")
    message(FATAL_ERROR "Set STM32CUBE_F4_ROOT to an STM32Cube_FW_F4 package containing the HAL and STM32469I-Discovery BSP")
endif()

set(cube "${STM32CUBE_F4_ROOT}")
set(hal "${cube}/Drivers/STM32F4xx_HAL_Driver")
set(core "${CMAKE_CURRENT_SOURCE_DIR}/Core")

add_executable(f469_ui
    ${core}/startup_stm32f469nihx.s
    ${core}/system_stm32f4xx.c
    ${core}/main.c
    ${hal}/Src/stm32f4xx_hal.c
    ${hal}/Src/stm32f4xx_hal_cortex.c
    ${hal}/Src/stm32f4xx_hal_flash.c
    ${hal}/Src/stm32f4xx_hal_flash_ex.c
    ${hal}/Src/stm32f4xx_hal_gpio.c
    ${hal}/Src/stm32f4xx_hal_pwr.c
    ${hal}/Src/stm32f4xx_hal_pwr_ex.c
    ${hal}/Src/stm32f4xx_hal_rcc.c
    ${hal}/Src/stm32f4xx_hal_rcc_ex.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/f469_ui.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/f469_face.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/f469_info.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/ui_theme.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/face_model.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/two_screen_controller.c
    ${CMAKE_SOURCE_DIR}/App/Src/ui/overview_model.c
)
target_include_directories(f469_ui PRIVATE
    ${core}
    ${hal}/Inc
    ${cube}/Drivers/CMSIS/Include
    ${cube}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/App/Inc
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl
)
target_compile_definitions(f469_ui PRIVATE STM32F469xx USE_HAL_DRIVER LV_CONF_INCLUDE_SIMPLE)
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
