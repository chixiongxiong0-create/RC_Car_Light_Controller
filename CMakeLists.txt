cmake_minimum_required(VERSION 3.20)

project("wio_ai" C CXX ASM)

include(cmake/st-project.cmake)

add_executable(${PROJECT_NAME})
add_st_target_properties(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    "App/Src/demo_vehicle_state.c"
    "App/Src/vehicle_state_source.c"
)
