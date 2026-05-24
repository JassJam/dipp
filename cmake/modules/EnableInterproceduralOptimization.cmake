include_guard(DIRECTORY)
include(CheckIPOSupported)

#
# usage:
#   enable_global_interprocedural_optimization()
#
function(enable_global_interprocedural_optimization)
    check_ipo_supported(RESULT result OUTPUT output)

    if (result)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON PARENT_SCOPE)
        message(STATUS "** Global IPO enabled: ${output}")
    else ()
        message(STATUS "** IPO is not supported: ${output}")
    endif ()
endfunction()

#
# usage:
#   target_enable_interprocedural_optimization(target_name)
#
function(target_enable_interprocedural_optimization TARGET_NAME)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_enable_interprocedural_optimization: Target '${TARGET_NAME}' does not exist")
    endif ()

    check_ipo_supported(RESULT result OUTPUT output)

    if (result)
        set_target_properties(${TARGET_NAME} PROPERTIES
                INTERPROCEDURAL_OPTIMIZATION TRUE
        )
        message(STATUS "** IPO enabled for target '${TARGET_NAME}': ${output}")
    else ()
        message(STATUS "** IPO is not supported for target '${TARGET_NAME}': ${output}")
    endif ()
endfunction()
