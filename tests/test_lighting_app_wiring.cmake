cmake_minimum_required(VERSION 3.20)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(READ "${REPO_ROOT}/App/Src/app.c" APP_SOURCE)

string(FIND "${APP_SOURCE}" "void App_Tick(uint32_t now_ms)" TICK_START)
if(TICK_START EQUAL -1)
    message(FATAL_ERROR "App_Tick was not found in App/Src/app.c")
endif()

string(SUBSTRING "${APP_SOURCE}" ${TICK_START} -1 APP_TAIL)
string(FIND "${APP_TAIL}" "void HAL_SPI_TxCpltCallback" TICK_END)
if(TICK_END EQUAL -1)
    message(FATAL_ERROR "App_Tick end marker was not found")
endif()
string(SUBSTRING "${APP_TAIL}" 0 ${TICK_END} APP_TICK)

string(REGEX MATCHALL "lighting_tick\\(" LIGHTING_CALLS "${APP_TICK}")
list(LENGTH LIGHTING_CALLS LIGHTING_CALL_COUNT)
if(NOT LIGHTING_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR
        "App_Tick must invoke lighting_tick exactly once; found ${LIGHTING_CALL_COUNT}")
endif()

string(REGEX MATCH
    "lighting_tick\\([ \t\r\n]*now_ms[ \t\r\n]*,[ \t\r\n]*real_state[ \t\r\n]*,[ \t\r\n]*low_battery[ \t\r\n]*\\)[ \t\r\n]*;"
    REAL_LIGHTING_CALL "${APP_TICK}")
if(REAL_LIGHTING_CALL STREQUAL "")
    message(FATAL_ERROR
        "App_Tick must invoke lighting_tick(now_ms, real_state, low_battery)")
endif()

string(REGEX MATCH
    "lighting_tick\\([^;]*presented[^;]*\\)[ \t\r\n]*;"
    PRESENTED_LIGHTING_CALL "${APP_TICK}")
if(NOT PRESENTED_LIGHTING_CALL STREQUAL "")
    message(FATAL_ERROR "App_Tick must never pass presented demo state to lighting")
endif()

string(REGEX MATCH
    "presented[ \t\r\n]*=[ \t\r\n]*vehicle_state_source_get\\([ \t\r\n]*&vehicle_source[ \t\r\n]*\\)[ \t\r\n]*;"
    PRESENTED_ASSIGNMENT "${APP_TICK}")
if(PRESENTED_ASSIGNMENT STREQUAL "")
    message(FATAL_ERROR "App_Tick presented-state assignment was not found")
endif()

string(FIND "${APP_TICK}" "${REAL_LIGHTING_CALL}" LIGHTING_POSITION)
string(FIND "${APP_TICK}" "${PRESENTED_ASSIGNMENT}" PRESENTED_POSITION)
if(NOT LIGHTING_POSITION LESS PRESENTED_POSITION)
    message(FATAL_ERROR
        "Physical lighting must run before presented demo state is acquired")
endif()
