# dear imgui: run two test executables (header and module builds of the same source) and check that their outputs match.
# Usage: cmake [-DEMULATOR=<emulator>] -DEXPECTED=<header build> -DACTUAL=<module build> -P compare_output.cmake

foreach(exe IN ITEMS EXPECTED ACTUAL)
    execute_process(COMMAND ${EMULATOR} "${${exe}}" OUTPUT_VARIABLE output_${exe} RESULT_VARIABLE result ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${${exe}} failed (${result}):\n${output_${exe}}${error}")
    endif()
endforeach()
message("${output_ACTUAL}")
if(NOT output_EXPECTED STREQUAL output_ACTUAL)
    message(FATAL_ERROR "Output differs from the header build:\n${output_EXPECTED}")
endif()
