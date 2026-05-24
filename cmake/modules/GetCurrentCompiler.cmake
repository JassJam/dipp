include_guard(DIRECTORY)
include(CMakeParseArguments)

# Helper to get the current compiler
# usage:
# get_current_compiler(
#   OUTPUT_VARIABLE
#   [INCLUDE_VERSION]         # Also includes compiler version info
#   [DEFAULT "UNKNOWN"]       # Sets a default value for unknown compilers
# )
#
function(get_current_compiler OUTPUT_VARIABLE)
    set(options INCLUDE_VERSION)
    set(oneValueArgs DEFAULT)
    set(multiValueArgs)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    #

    # Set default unknown value
    if (NOT DEFINED ARG_DEFAULT)
        set(UNKNOWN_COMPILER "UNKNOWN")
    else ()
        set(UNKNOWN_COMPILER "${ARG_DEFAULT}")
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(DETECTED_COMPILER "MSVC")
        set(COMPILER_VERSION ${MSVC_VERSION})

    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang.*")
        # Check if this is actually Emscripten (which reports as Clang)
        if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR
                CMAKE_TOOLCHAIN_FILE MATCHES "emscripten" OR
                CMAKE_CXX_COMPILER MATCHES "em\\+\\+")
            set(DETECTED_COMPILER "EMSCRIPTEN")
        elseif (MSVC)
            set(DETECTED_COMPILER "CLANG-MSVC")
        else ()
            set(DETECTED_COMPILER "CLANG")
        endif ()
        set(COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})

    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(DETECTED_COMPILER "GCC")
        set(COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})

    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Emscripten" OR EMSCRIPTEN)
        set(DETECTED_COMPILER "EMSCRIPTEN")
        set(COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})

    else ()
        message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
        set(DETECTED_COMPILER "${UNKNOWN_COMPILER}")
        set(COMPILER_VERSION "")
    endif ()

    # Add version info if requested
    if (ARG_INCLUDE_VERSION AND
            NOT DETECTED_COMPILER STREQUAL "${UNKNOWN_COMPILER}" AND
            DEFINED COMPILER_VERSION)
        set(DETECTED_COMPILER "${DETECTED_COMPILER}-${COMPILER_VERSION}")
    endif ()

    # Set the output variable in the parent scope
    set(${OUTPUT_VARIABLE} "${DETECTED_COMPILER}" PARENT_SCOPE)
endfunction()