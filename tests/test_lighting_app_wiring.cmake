cmake_minimum_required(VERSION 3.20)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(READ "${REPO_ROOT}/App/Src/app.c" APP_SOURCE)

string(FIND "${APP_SOURCE}" "void App_Tick(uint32_t now_ms)" TICK_START)
if(TICK_START EQUAL -1)
    message(FATAL_ERROR "App_Tick was not found in App/Src/app.c")
endif()

string(SUBSTRING "${APP_SOURCE}" ${TICK_START} -1 APP_TAIL)
set(APP_TICK "${APP_TAIL}")

string(REGEX MATCH
    "static[ \t\r\n]+bool[ \t\r\n]+submit_lighting\\([ \t\r\n]*uint32_t[ \t\r\n]+now_ms[ \t\r\n]*,[ \t\r\n]*const[ \t\r\n]+Ws2812Frame[ \t\r\n]*\\*[ \t\r\n]*frame[ \t\r\n]*,[ \t\r\n]*void[ \t\r\n]*\\*[ \t\r\n]*ctx[ \t\r\n]*\\)"
    ATOMIC_SUBMIT_SIGNATURE "${APP_SOURCE}")
if(ATOMIC_SUBMIT_SIGNATURE STREQUAL "")
    message(FATAL_ERROR
        "submit_lighting must accept uint32_t, const Ws2812Frame *, and void *")
endif()

string(REGEX MATCHALL
    "ws2812_port_submit\\([ \t\r\n]*now_ms[ \t\r\n]*,[ \t\r\n]*frame[ \t\r\n]*\\)"
    PORT_SUBMIT_CALLS "${APP_SOURCE}")
list(LENGTH PORT_SUBMIT_CALLS PORT_SUBMIT_CALL_COUNT)
if(NOT PORT_SUBMIT_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR
        "app.c must call ws2812_port_submit(now_ms, frame) exactly once; found ${PORT_SUBMIT_CALL_COUNT}")
endif()

foreach(callback IN ITEMS
        HAL_SPI_TxCpltCallback
        HAL_SPI_ErrorCallback
        HAL_SPI_AbortCpltCallback)
    string(FIND "${APP_SOURCE}" "${callback}" callback_at)
    if(NOT callback_at EQUAL -1)
        message(FATAL_ERROR "Obsolete SPI callback remains in app.c: ${callback}")
    endif()
endforeach()

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

string(REGEX MATCH
    "lighting_output_port_apply\\([ \t\r\n]*front_duty[ \t\r\n]*,[ \t\r\n]*roof_spot_duty[ \t\r\n]*\\)"
    PHYSICAL_APPLY_CALL "${APP_SOURCE}")
if(PHYSICAL_APPLY_CALL STREQUAL "")
    message(FATAL_ERROR "PF3/PE10 lighting apply wiring was not preserved")
endif()

string(REGEX MATCH
    "led_current_ma[ \t\r\n]*=[ \t\r\n]*lighting_service_tick\\("
    DIAGNOSTIC_CURRENT_ASSIGNMENT "${APP_SOURCE}")
if(DIAGNOSTIC_CURRENT_ASSIGNMENT STREQUAL "")
    message(FATAL_ERROR "Lighting diagnostic-current assignment was not preserved")
endif()

string(FIND "${APP_SOURCE}"
    "diagnostics_watchdog_mark(DIAG_PROGRESS_LED);" WATCHDOG_MARK_AT)
if(WATCHDOG_MARK_AT EQUAL -1)
    message(FATAL_ERROR "Lighting watchdog progress mark was not preserved")
endif()

string(FIND "${APP_SOURCE}" "vehicle_state_source_init(&vehicle_source);"
    VEHICLE_SOURCE_INIT_AT)
string(FIND "${APP_SOURCE}" "diagnostics_init();" DIAGNOSTICS_INIT_AT)
string(FIND "${APP_SOURCE}" "ws2812_port_init();" WS2812_INIT_AT)
if(VEHICLE_SOURCE_INIT_AT EQUAL -1 OR DIAGNOSTICS_INIT_AT EQUAL -1 OR
   WS2812_INIT_AT EQUAL -1 OR
   NOT VEHICLE_SOURCE_INIT_AT LESS WS2812_INIT_AT OR
   NOT DIAGNOSTICS_INIT_AT LESS WS2812_INIT_AT)
    message(FATAL_ERROR
        "ws2812_port_init must remain after real-state source and diagnostics initialization")
endif()
