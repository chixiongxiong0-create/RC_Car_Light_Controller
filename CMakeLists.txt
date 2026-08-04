cmake_minimum_required(VERSION 3.20)

project("wio_ai" C CXX ASM)

set(WIO_AI_APP_LINKER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/STM32H725AEIX_PSRAM.ld")

include(cmake/st-project.cmake)

add_executable(${PROJECT_NAME})
add_st_target_properties(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    "App/Src/demo_vehicle_state.c"
    "App/Src/vehicle_state_source.c"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Fonts"
    "${CMAKE_CURRENT_SOURCE_DIR}/Utilities/lcd"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Components/ili9341"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Components/aps6408"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Components"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Components/Common"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Components/W25Q128"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Components/LCD"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Components/LOG_UART3"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/wio_lite_ai"
)

target_link_libraries(${PROJECT_NAME} PRIVATE m)

add_subdirectory(boot_stub)
