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
    test_lvgl_port_math.c
    test_display_metrics.c
    test_input_manager.c
    test_button_input.c
    test_dashboard_format.c
    test_face_model.c
    ../App/Src/app_time.c
    ../App/Src/msp/msp_codec.c
    ../App/Src/msp/msp_client.c
    ../App/Src/vehicle_state.c
    ../App/Src/platform/msp_uart.c
    ../App/Src/platform/lvgl_port_math.c
    ../App/Src/platform/display_metrics.c
    ../App/Src/input_manager.c
    ../App/Src/platform/button_input.c
    ../App/Src/ui/dashboard_format.c
    ../App/Src/ui/face_model.c
)
target_include_directories(unit_tests PRIVATE fakes ../App/Inc)
target_compile_definitions(unit_tests PRIVATE MSP_UART_HOST_TEST=1)

add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME lvgl_make_manifest
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/test_lvgl_make_manifest.ps1)
add_test(NAME button_polling_manifest
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/test_button_polling_manifest.ps1)
