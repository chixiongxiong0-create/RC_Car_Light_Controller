cmake_minimum_required(VERSION 3.20)
project(app_config_probe C)

if(NOT DEFINED PROJECT_ROOT OR
   NOT DEFINED PROBE_REAR_PIXEL_COUNT OR
   NOT DEFINED PROBE_LED_PIXEL_COUNT)
    message(FATAL_ERROR "app config probe inputs are required")
endif()

add_executable(app_config_probe probe.c)
target_include_directories(app_config_probe PRIVATE "${PROJECT_ROOT}/App/Inc")
target_compile_definitions(app_config_probe PRIVATE
    APP_REAR_PIXEL_COUNT=${PROBE_REAR_PIXEL_COUNT}
    APP_LED_PIXEL_COUNT=${PROBE_LED_PIXEL_COUNT})
