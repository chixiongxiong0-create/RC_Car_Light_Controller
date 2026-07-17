cmake_minimum_required(VERSION 3.20)
project(wio_ui_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

add_executable(unit_tests
    test_main.c
    test_app_time.c
    test_msp_codec.c
    test_msp_client.c
    test_vehicle_state.c
    test_msp_uart.c
    ../App/Src/app_time.c
    ../App/Src/msp/msp_codec.c
    ../App/Src/msp/msp_client.c
    ../App/Src/vehicle_state.c
    ../App/Src/platform/msp_uart.c
)
target_include_directories(unit_tests PRIVATE fakes ../App/Inc)
target_compile_definitions(unit_tests PRIVATE MSP_UART_HOST_TEST=1)

add_test(NAME unit_tests COMMAND unit_tests)
