function(run_config_case name rear_count total_count expect_success)
    set(case_build "${TEST_BINARY_DIR}/${name}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
                -S "${PROBE_SOURCE_DIR}"
                -B "${case_build}"
                -G "${TEST_GENERATOR}"
                "-DPROJECT_ROOT=${PROJECT_ROOT}"
                "-DPROBE_REAR_PIXEL_COUNT=${rear_count}"
                "-DPROBE_LED_PIXEL_COUNT=${total_count}"
        RESULT_VARIABLE configure_result
        OUTPUT_VARIABLE configure_output
        ERROR_VARIABLE configure_error)

    set(case_succeeded FALSE)
    if(configure_result EQUAL 0)
        execute_process(
            COMMAND "${CMAKE_COMMAND}" --build "${case_build}"
            RESULT_VARIABLE build_result
            OUTPUT_VARIABLE build_output
            ERROR_VARIABLE build_error)
        if(build_result EQUAL 0)
            set(case_succeeded TRUE)
        endif()
    endif()

    if(expect_success AND NOT case_succeeded)
        message(FATAL_ERROR
            "${name} should compile but failed:\n${configure_output}\n"
            "${configure_error}\n${build_output}\n${build_error}")
    endif()
    if(NOT expect_success AND case_succeeded)
        message(FATAL_ERROR "${name} should be rejected but compiled")
    endif()
endfunction()

run_config_case(valid_minimum 4 4 TRUE)
run_config_case(valid_maximum 4 30 TRUE)
run_config_case(invalid_rear_override 5 5 FALSE)
run_config_case(invalid_total_below_rear 4 3 FALSE)
run_config_case(invalid_total_above_max 4 31 FALSE)
