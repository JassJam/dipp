include_guard(DIRECTORY)
include(GetCurrentCompiler)

#
# usage:
#   target_configure_exceptions(TARGET_NAME [ON/OFF])
#
function(target_configure_exceptions TARGET_NAME ON_OFF)
    if (GLOBAL_EXCEPTIONS_SET)
        message(TRACE "Global exceptions are already enabled, ignoring target-specific settings")
        return()
    endif ()

    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "Target ${TARGET_NAME} does not exist")
    endif ()

    _configure_exceptions(COMPILE_FLAGS LINK_FLAGS ON_OFF)
    if (COMPILE_FLAGS)
        target_compile_options(${TARGET_NAME} PRIVATE ${COMPILE_FLAGS})
    endif ()
    if (LINK_FLAGS)
        target_link_options(${TARGET_NAME} PRIVATE ${LINK_FLAGS})
    endif ()
endfunction()

#
# usage:
#   configure_global_exceptions([ON/OFF])
#
function(configure_global_exceptions ON_OFF)
    _configure_exceptions(COMPILE_FLAGS LINK_FLAGS ${ON_OFF})
    if (COMPILE_FLAGS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}" PARENT_SCOPE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILE_FLAGS}" PARENT_SCOPE)
    endif ()
    if (LINK_FLAGS)
        foreach(flag ${LINK_FLAGS})
            add_link_options(${flag})
        endforeach()
    endif ()

    set(GLOBAL_EXCEPTIONS_SET TRUE PARENT_SCOPE)
    message(STATUS "Global exceptions configured: ${ON_OFF}")
endfunction()

#

# Helper function to configure exceptions flags based on compiler
function(_configure_exceptions OUT_COMPILE_FLAGS OUT_LINK_FLAGS ON_OFF)
    get_current_compiler(CURRENT_COMPILER)
    if (CURRENT_COMPILER STREQUAL "MSVC" OR CURRENT_COMPILER STREQUAL "CLANG-MSVC")
        if (ON_OFF)
            set(${OUT_COMPILE_FLAGS} "/EHsc" PARENT_SCOPE)
        else ()
            set(${OUT_COMPILE_FLAGS} "/EHs-c-" PARENT_SCOPE)
        endif ()
    elseif (CURRENT_COMPILER MATCHES "CLANG.*|GCC|EMSCRIPTEN")
        if (ON_OFF)
            set(${OUT_COMPILE_FLAGS} "-fexceptions" PARENT_SCOPE)
            if (CURRENT_COMPILER MATCHES "EMSCRIPTEN")
                set(${OUT_LINK_FLAGS} "-fexceptions" "SHELL:-s DISABLE_EXCEPTION_CATCHING=0" PARENT_SCOPE)
            endif ()
        else ()
            set(${OUT_COMPILE_FLAGS} "-fno-exceptions" PARENT_SCOPE)
            if (CURRENT_COMPILER MATCHES "EMSCRIPTEN")
                set(${OUT_LINK_FLAGS} "SHELL:-s DISABLE_EXCEPTION_CATCHING=1" PARENT_SCOPE)
            endif ()
        endif ()
    else ()
        message(WARNING "Unknown compiler ${CURRENT_COMPILER}, not applying exceptions settings")
    endif ()
endfunction()