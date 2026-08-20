get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(TARGET_FILES
    "${PROJECT_ROOT}/App/Src/platform/ws2812_port.c"
    "${PROJECT_ROOT}/Core/Src/tim.c"
    "${PROJECT_ROOT}/Core/Src/stm32h7xx_it.c"
    "${PROJECT_ROOT}/Core/Src/stm32h7xx_hal_msp.c")

set(TARGET_TEXT "")
foreach(target_file IN LISTS TARGET_FILES)
    file(READ "${target_file}" file_text)
    string(APPEND TARGET_TEXT "\n${file_text}")
endforeach()
file(READ "${PROJECT_ROOT}/Core/Src/tim.c" TIM_TEXT)

function(require_text literal)
    string(FIND "${TARGET_TEXT}" "${literal}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Required WS2812 timer wiring is missing: ${literal}")
    endif()
endfunction()

function(reject_text literal)
    string(FIND "${TARGET_TEXT}" "${literal}" found_at)
    if(NOT found_at EQUAL -1)
        message(FATAL_ERROR "Rejected legacy WS2812 wiring is present: ${literal}")
    endif()
endfunction()

function(require_safe_low_before_init port pins)
    set(write_call "HAL_GPIO_WritePin(${port}, ${pins}, GPIO_PIN_RESET)")
    string(FIND "${TIM_TEXT}" "${write_call}" write_at)
    if(write_at EQUAL -1)
        message(FATAL_ERROR "Missing safe-low write: ${write_call}")
    endif()

    string(SUBSTRING "${TIM_TEXT}" ${write_at} -1 after_write)
    string(FIND "${after_write}" "GPIO_InitStruct.Pin = ${pins}" pin_at)
    string(FIND "${after_write}" "HAL_GPIO_Init(${port}, &GPIO_InitStruct)" init_at)
    if(pin_at EQUAL -1 OR init_at EQUAL -1 OR pin_at GREATER init_at)
        message(FATAL_ERROR
            "Safe-low write is not before the corresponding ${port} ${pins} init")
    endif()
endfunction()

require_text("GPIO_AF1_TIM2")
require_text("GPIO_AF2_TIM3")
require_text("DMA_REQUEST_TIM2_UP")
require_text("DMA_REQUEST_TIM3_UP")
require_text("TIM_DMABASE_CCR1")
require_text("TIM_DMABURSTLENGTH_2TRANSFERS")
require_text("GPIO_PIN_0")
require_text("GPIO_PIN_3")
require_text("GPIO_PIN_4 | GPIO_PIN_5")
require_text("ws2812_timer_fail_safe")
require_text("ws2812_dma_fail_safe")
require_text("TIM_DMABURSTLENGTH_2TRANSFERS,\n            (uint32_t)(slots * 2u)) != HAL_OK) {\n        ws2812_pair_stop(pair, NULL)")
reject_text("HAL_SPI_Transmit_DMA")

require_safe_low_before_init("GPIOA" "GPIO_PIN_0")
require_safe_low_before_init("GPIOB" "GPIO_PIN_3")
require_safe_low_before_init("GPIOB" "GPIO_PIN_4 | GPIO_PIN_5")
