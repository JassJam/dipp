include_guard(DIRECTORY)

include(GetCurrentCompiler)

# 
# usage:
# switch_on_compiler(
#   output_variable
#   [MSVC value ...]
#   [CLANG value ...]
#   [GCC value ...]
#   [EMSCRIPTEN value ...]
# )
function(switch_on_compiler OUTPUT_VARIABLE)
    set(multiValueArgs
            MSVC
            CLANG
            GCC
            EMSCRIPTEN
    )
    cmake_parse_arguments(ARG "" "" "${multiValueArgs}" ${ARGN})

    get_current_compiler(CURRENT_COMPILER)

    if ("${CURRENT_COMPILER}" STREQUAL "MSVC")
        set(SELECTED_VALUE "${ARG_MSVC}")
    elseif ("${CURRENT_COMPILER}" MATCHES "CLANG")
        set(SELECTED_VALUE "${ARG_CLANG}")
    elseif ("${CURRENT_COMPILER}" STREQUAL "GCC")
        set(SELECTED_VALUE "${ARG_GCC}")
    elseif ("${CURRENT_COMPILER}" STREQUAL "EMSCRIPTEN")
        set(SELECTED_VALUE "${ARG_EMSCRIPTEN}")
    else ()
        message(STATUS "switch_on_compiler() called with unsupported compiler: ${CURRENT_COMPILER}")
    endif ()

    if (SELECTED_VALUE)
        set(${OUTPUT_VARIABLE} ${SELECTED_VALUE} PARENT_SCOPE)
    endif ()
endfunction()