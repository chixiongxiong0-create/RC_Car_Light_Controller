cmake_minimum_required(VERSION 3.20)
project(wio_ui_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

add_executable(unit_tests
    test_main.c
    test_app_time.c
    ../App/Src/app_time.c
)
target_include_directories(unit_tests PRIVATE ../App/Inc)

add_test(NAME unit_tests COMMAND unit_tests)
