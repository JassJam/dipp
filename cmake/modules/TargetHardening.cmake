include_guard(DIRECTORY)
include(GetCurrentCompiler)

set_property(GLOBAL PROPERTY PROJECT_GLOBAL_HARDENING_ENABLED FALSE)

#
# usage:
# target_enable_hardening(
#   TARGET_NAME
# 	[PRIVATE|PUBLIC|INTERFACE]
# )
#
function(target_enable_hardening TARGET_NAME SCOPE_NAME)
    # Call once
    get_property(already_registered GLOBAL PROPERTY PROJECT_GLOBAL_HARDENING_ENABLED)
    if (already_registered)
        return()
    endif ()

    if (NOT TARGET_NAME OR NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_enable_hardening() called without TARGET")
    endif ()
    if (NOT SCOPE_NAME)
        set(SCOPE_NAME PRIVATE)
    elseif (NOT ${SCOPE_NAME} IN_LIST CMAKE_TARGET_SCOPE_TYPES)
        message(FATAL_ERROR "Invalid SCOPE_NAME '${SCOPE_NAME}' for target_enable_hardening()")
    endif ()

    _should_enable_ubsan_minimal_runtime(ENABLE_UBSAN_MINIMAL_RUNTIME)
    _get_hardening_options(NEW_COMPILE_OPTIONS NEW_LINK_OPTIONS NEW_CXX_DEFINITIONS)

    message(STATUS "** Hardening Compiler Flags: ${NEW_COMPILE_OPTIONS}")
    message(STATUS "** Hardening Linker Flags: ${NEW_LINK_OPTIONS}")
    message(STATUS "** Hardening Compiler Defines: ${NEW_CXX_DEFINITIONS}")

    # if NEW_COMPILE_OPTIONS is not empty, set it
    if (NOT "${NEW_COMPILE_OPTIONS}" STREQUAL "")
        target_compile_options(${TARGET_NAME} ${SCOPE_NAME} ${NEW_COMPILE_OPTIONS})
    endif ()

    # if NEW_LINK_OPTIONS is not empty, set it
    if (NOT "${NEW_LINK_OPTIONS}" STREQUAL "")
        target_link_options(${TARGET_NAME} ${SCOPE_NAME} ${NEW_LINK_OPTIONS})
    endif ()

    # if NEW_CXX_DEFINITIONS is not empty, set it
    if (NOT "${NEW_CXX_DEFINITIONS}" STREQUAL "")
        target_compile_definitions(${TARGET_NAME} ${SCOPE_NAME} ${NEW_CXX_DEFINITIONS})
    endif ()
endfunction()

#
# usage:
# enable_global_hardening()
#
function(enable_global_hardening)
    # Call once
    get_property(already_registered GLOBAL PROPERTY PROJECT_GLOBAL_HARDENING_ENABLED)
    if (already_registered)
        return()
    endif ()

    message(STATUS "** Enable global hardening to all targets and all dependencies")

    _should_enable_ubsan_minimal_runtime(ENABLE_UBSAN_MINIMAL_RUNTIME)
    _get_hardening_options(NEW_COMPILE_OPTIONS NEW_LINK_OPTIONS NEW_CXX_DEFINITIONS)

    message(STATUS "** Hardening Compiler Flags: ${NEW_COMPILE_OPTIONS}")
    message(STATUS "** Hardening Linker Flags: ${NEW_LINK_OPTIONS}")
    message(STATUS "** Hardening Compiler Defines: ${NEW_CXX_DEFINITIONS}")

    message(STATUS "** Setting hardening options globally for all dependencies")

    # Set global compile options
    if (NOT "${NEW_COMPILE_OPTIONS}" STREQUAL "")
        string(JOIN " " COMPILE_FLAGS_STR ${NEW_COMPILE_OPTIONS})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS_STR}" CACHE STRING "Global CXX flags with hardening" FORCE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILE_FLAGS_STR}" CACHE STRING "Global C flags with hardening" FORCE)
    endif ()

    # Set global link options
    if (NOT "${NEW_LINK_OPTIONS}" STREQUAL "")
        string(JOIN " " LINK_FLAGS_STR ${NEW_LINK_OPTIONS})
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINK_FLAGS_STR}" CACHE STRING "Global EXE linker flags with hardening" FORCE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINK_FLAGS_STR}" CACHE STRING "Global SHARED linker flags with hardening" FORCE)
    endif ()

    # Set global compile definitions
    if (NOT "${NEW_CXX_DEFINITIONS}" STREQUAL "")
        foreach (DEFINITION ${NEW_CXX_DEFINITIONS})
            add_compile_definitions(${DEFINITION})
        endforeach ()
    endif ()

    set_property(GLOBAL PROPERTY PROJECT_GLOBAL_HARDENING_ENABLED TRUE)
endfunction()

#

# Helper function to determine if UBSan minimal runtime should be enabled
function(_should_enable_ubsan_minimal_runtime RESULT_VAR)
    if (NOT SUPPORTS_UBSAN
            OR ENABLE_UBSAN
            OR ENABLE_ASAN
            OR ENABLE_TSAN
            OR ENABLE_LSAN
            OR ENABLE_MSAN)
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
    else ()
        set(${RESULT_VAR} TRUE PARENT_SCOPE)
    endif ()
endfunction()

# Helper function to configure MSVC hardening flags
function(_configure_msvc_hardening COMPILE_OPTIONS_VAR LINK_OPTIONS_VAR DEFINITIONS_VAR)
    # Check if Edit and Continue is enabled globally (for compatibility)
    if (ENABLE_EDIT_AND_CONTINUE)
        message(STATUS "*** Hardening MSVC flags: /DYNAMICBASE /NXCOMPAT /CETCOMPAT (Control Flow Guard disabled due to Edit and Continue)")
        # Skip /guard:cf when Edit and Continue is enabled
    else ()
        message(STATUS "*** Hardening MSVC flags: /DYNAMICBASE /guard:cf /NXCOMPAT /CETCOMPAT")
        # /guard:cf is a compiler flag for Control Flow Guard
        list(APPEND ${COMPILE_OPTIONS_VAR} /guard:cf)
    endif ()

    # /DYNAMICBASE, /NXCOMPAT, /CETCOMPAT are linker flags
    list(APPEND ${LINK_OPTIONS_VAR} /DYNAMICBASE /NXCOMPAT /CETCOMPAT)

    set(${COMPILE_OPTIONS_VAR} ${${COMPILE_OPTIONS_VAR}} PARENT_SCOPE)
    set(${LINK_OPTIONS_VAR} ${${LINK_OPTIONS_VAR}} PARENT_SCOPE)
endfunction()

# Helper function to configure GCC/Clang hardening flags
function(_configure_gcc_clang_hardening COMPILE_OPTIONS_VAR LINK_OPTIONS_VAR DEFINITIONS_VAR CURRENT_COMPILER)
    message(STATUS "*** GLIBC++ Assertions (vector[], string[], ...) enabled")
    list(APPEND ${DEFINITIONS_VAR} _GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC _GLIBCXX_ASSERTIONS)

    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        message(STATUS "*** g++/clang _FORTIFY_SOURCE=3 enabled")
        list(APPEND ${COMPILE_OPTIONS_VAR} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3)
    endif()

    # Stack protector
    check_cxx_compiler_flag(-fstack-protector-strong STACK_PROTECTOR)
    if (STACK_PROTECTOR)
        message(STATUS "*** g++/clang -fstack-protector-strong enabled")
        list(APPEND ${COMPILE_OPTIONS_VAR} -fstack-protector-strong)
    else ()
        message(STATUS "*** g++/clang -fstack-protector-strong NOT enabled (not supported)")
    endif ()

    # Control flow protection
    check_cxx_compiler_flag(-fcf-protection CF_PROTECTION)
    if (CF_PROTECTION)
        message(STATUS "*** g++/clang -fcf-protection enabled")
        list(APPEND ${COMPILE_OPTIONS_VAR} -fcf-protection)
    else ()
        message(STATUS "*** g++/clang -fcf-protection NOT enabled (not supported)")
    endif ()

    # Stack clash protection
    check_cxx_compiler_flag(-fstack-clash-protection CLASH_PROTECTION)
    if (CLASH_PROTECTION)
        if (LINUX OR "${CURRENT_COMPILER}" MATCHES "GCC")
            message(STATUS "*** g++/clang -fstack-clash-protection enabled")
            list(APPEND ${COMPILE_OPTIONS_VAR} -fstack-clash-protection)
        else ()
            message(STATUS "*** g++/clang -fstack-clash-protection NOT enabled (clang on non-Linux)")
        endif ()
    else ()
        message(STATUS "*** g++/clang -fstack-clash-protection NOT enabled (not supported)")
    endif ()

    # UBSan minimal runtime - only enable if compatible with other sanitizers
    _should_enable_ubsan_minimal_runtime(SHOULD_ENABLE_MINIMAL_RUNTIME)
    if (SHOULD_ENABLE_MINIMAL_RUNTIME)
        check_cxx_compiler_flag("-fsanitize=undefined -fno-sanitize-recover=undefined -fsanitize-minimal-runtime"
                MINIMAL_RUNTIME)

        if (MINIMAL_RUNTIME)
            list(APPEND ${COMPILE_OPTIONS_VAR} -fsanitize=undefined -fsanitize-minimal-runtime -fno-sanitize-recover=undefined)
            list(APPEND ${LINK_OPTIONS_VAR} -fsanitize=undefined -fsanitize-minimal-runtime -fno-sanitize-recover=undefined)
            message(STATUS "*** ubsan minimal runtime enabled")
        else ()
            message(STATUS "*** ubsan minimal runtime NOT enabled (not supported)")
        endif ()
    else ()
        message(STATUS "*** ubsan minimal runtime NOT enabled (incompatible with other sanitizers)")
    endif ()

    set(${COMPILE_OPTIONS_VAR} ${${COMPILE_OPTIONS_VAR}} PARENT_SCOPE)
    set(${LINK_OPTIONS_VAR} ${${LINK_OPTIONS_VAR}} PARENT_SCOPE)
    set(${DEFINITIONS_VAR} ${${DEFINITIONS_VAR}} PARENT_SCOPE)
endfunction()

# Helper function to get hardening options for current compiler
function(_get_hardening_options COMPILE_OPTIONS_VAR LINK_OPTIONS_VAR DEFINITIONS_VAR)
    get_current_compiler(CURRENT_COMPILER)

    set(NEW_LINK_OPTIONS "")
    set(NEW_COMPILE_OPTIONS "")
    set(NEW_CXX_DEFINITIONS "")

    if ("${CURRENT_COMPILER}" MATCHES "MSVC")
        _configure_msvc_hardening(NEW_COMPILE_OPTIONS NEW_LINK_OPTIONS NEW_CXX_DEFINITIONS)
    elseif ("${CURRENT_COMPILER}" MATCHES "CLANG|GCC|EMSCRIPTEN")
        _configure_gcc_clang_hardening(NEW_COMPILE_OPTIONS NEW_LINK_OPTIONS NEW_CXX_DEFINITIONS "${CURRENT_COMPILER}")
    else ()
        message(STATUS "*** ubsan minimal runtime NOT enabled (not requested)")
    endif ()

    set(${COMPILE_OPTIONS_VAR} ${NEW_COMPILE_OPTIONS} PARENT_SCOPE)
    set(${LINK_OPTIONS_VAR} ${NEW_LINK_OPTIONS} PARENT_SCOPE)
    set(${DEFINITIONS_VAR} ${NEW_CXX_DEFINITIONS} PARENT_SCOPE)
endfunction()