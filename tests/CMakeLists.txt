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
    test_low_battery_policy.c
    test_showcase_model.c
    test_led_controller.c
    ../App/Inc/led/ws2812_frame.h
    test_lighting_controller.c
    test_lighting_service.c
    test_lighting_output_port.c
    test_ws2812_encoder.c
    test_diagnostics.c
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
    ../App/Src/ui/low_battery_policy.c
    ../App/Src/ui/showcase_model.c
    ../App/Src/led/led_controller.c
    ../App/Src/lighting/lighting_controller.c
    ../App/Src/lighting/lighting_service.c
    ../App/Src/platform/lighting_output_port.c
    ../App/Src/platform/ws2812_port.c
    ../App/Src/diagnostics.c
    ../App/Src/platform/touch_probe.c
)
target_include_directories(unit_tests PRIVATE fakes ../App/Inc)
target_compile_definitions(unit_tests PRIVATE MSP_UART_HOST_TEST=1 WS2812_HOST_TEST=1 TOUCH_PROBE_HOST_TEST=1 DIAGNOSTICS_HOST_TEST=1 LIGHTING_OUTPUT_PORT_HOST_TEST=1)

add_test(NAME unit_tests COMMAND unit_tests)

add_executable(demo_vehicle_state_tests
    test_demo_vehicle_state.c
    ../App/Src/demo_vehicle_state.c
)
target_include_directories(demo_vehicle_state_tests PRIVATE ../App/Inc)
add_test(NAME demo_vehicle_state_tests COMMAND demo_vehicle_state_tests)

add_executable(vehicle_state_source_tests
    test_vehicle_state_source.c
    ../App/Src/demo_vehicle_state.c
    ../App/Src/vehicle_state_source.c
)
target_include_directories(vehicle_state_source_tests PRIVATE ../App/Inc)
add_test(NAME vehicle_state_source_tests COMMAND vehicle_state_source_tests)

add_executable(low_battery_source_tests
    test_low_battery_source.c
    ../App/Src/demo_vehicle_state.c
    ../App/Src/vehicle_state.c
    ../App/Src/vehicle_state_source.c
    ../App/Src/ui/low_battery_policy.c
)
target_include_directories(low_battery_source_tests PRIVATE ../App/Inc)
add_test(NAME low_battery_source_tests COMMAND low_battery_source_tests)

if(NOT MSVC)
    target_link_libraries(demo_vehicle_state_tests PRIVATE m)
    target_link_libraries(vehicle_state_source_tests PRIVATE m)
    target_link_libraries(low_battery_source_tests PRIVATE m)
endif()

add_test(NAME lvgl_make_manifest
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/test_lvgl_make_manifest.ps1)
add_test(NAME button_polling_manifest
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/test_button_polling_manifest.ps1)
add_test(NAME watchdog_manifest
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/test_watchdog_manifest.ps1)
add_test(NAME demo_safety_manifest
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/test_demo_safety_manifest.ps1)
add_test(NAME lighting_app_wiring
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/test_lighting_app_wiring.cmake)
add_test(NAME app_config_compile_contract
    COMMAND ${CMAKE_COMMAND}
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/..
            -DPROBE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/app_config_probe
            -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/app_config_probe_builds
            -DTEST_GENERATOR=${CMAKE_GENERATOR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/test_app_config_compile.cmake)
add_test(NAME boot_stub_manifest
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/test_boot_stub_manifest.ps1)
