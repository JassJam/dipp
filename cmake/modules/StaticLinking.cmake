include_guard(DIRECTORY)
include(GetCurrentCompiler)

#
# usage:
# target_enable_static_linking(
#   TARGET_NAME
#   [PRIVATE|PUBLIC|INTERFACE]
# )
#
function(target_enable_static_linking TARGET_NAME SCOPE_NAME)
    # if STATIC_LINKING_ENABLED is already set, skip re-enabling
    if (DEFINED STATIC_LINKING_ENABLED AND STATIC_LINKING_ENABLED)
        message(STATUS "Static linking already enabled for target: ${TARGET_NAME}")
        return()
    endif ()

    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_enable_static_linking: TARGET argument is required")
    endif ()

    if (NOT SCOPE_NAME)
        set(SCOPE_NAME PRIVATE)
    elseif (NOT ${SCOPE_NAME} IN_LIST CMAKE_TARGET_SCOPE_TYPES)
        message(FATAL_ERROR "target_enable_static_linking: Invalid scope '${SCOPE_NAME}' specified. Must be one of: ${CMAKE_TARGET_SCOPE_TYPES}.")
    endif ()

    # Get current compiler
    get_current_compiler(CURRENT_COMPILER)

    # Apply compiler-specific static linking
    if (CURRENT_COMPILER MATCHES "CLANG.*|GCC")
        target_link_options(${TARGET_NAME} ${SCOPE_NAME} "-static-libstdc++" "-static-libgcc")
        message(STATUS "Enabling static runtime linking for ${CURRENT_COMPILER} target: ${TARGET_NAME}")
    elseif (CURRENT_COMPILER STREQUAL "MSVC")
        set_target_properties(${TARGET_NAME} PROPERTIES
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
        message(STATUS "Enabling static runtime linking for MSVC target: ${TARGET_NAME}")
    elseif (CURRENT_COMPILER STREQUAL "INTEL")
        target_link_options(${TARGET_NAME} ${SCOPE_NAME} "-static-intel")
        message(STATUS "Enabling static runtime linking for Intel target: ${TARGET_NAME}")
    elseif (CURRENT_COMPILER STREQUAL "EMSCRIPTEN")
        # Emscripten static linking: link C++ standard library statically
        target_link_options(${TARGET_NAME} ${SCOPE_NAME} "-static-libstdc++")
        # For more portable/standalone WebAssembly output
        target_link_options(${TARGET_NAME} ${SCOPE_NAME} "SHELL:-s STANDALONE_WASM=1")
        target_link_options(${TARGET_NAME} ${SCOPE_NAME} "SHELL:-s WASM=1")
        message(STATUS "Enabling static runtime linking for Emscripten target: ${TARGET_NAME}")
    else ()
        message(WARNING "Static runtime linking not supported for compiler: ${CURRENT_COMPILER}")
    endif ()
endfunction()

#
# usage:
# enable_static_linking()
#
function(enable_static_linking)
    get_current_compiler(CURRENT_COMPILER)

    message(STATUS "Enabling static linking of runtime libraries for ${CURRENT_COMPILER}")

    if (CURRENT_COMPILER STREQUAL "CLANG" OR CURRENT_COMPILER MATCHES "GCC")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        message(STATUS "Static linking flags applied for GCC/Clang")
    elseif (CURRENT_COMPILER STREQUAL "MSVC" OR CURRENT_COMPILER STREQUAL "CLANG-MSVC")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library" FORCE)
        message(STATUS "Static runtime linking enabled for ${CURRENT_COMPILER}")
    elseif (CURRENT_COMPILER STREQUAL "INTEL")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-intel")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-intel")
        message(STATUS "Static linking flags applied for Intel compiler")
    elseif (CURRENT_COMPILER STREQUAL "EMSCRIPTEN")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -s STANDALONE_WASM=1 -s WASM=1")
        message(STATUS "Static linking flags applied for Emscripten")
    else ()
        message(WARNING "Static linking not configured for compiler: ${CURRENT_COMPILER}")
    endif ()

    set(STATIC_LINKING_ENABLED TRUE PARENT_SCOPE)
    message(STATUS "Static linking of runtime libraries enabled")
endfunction()