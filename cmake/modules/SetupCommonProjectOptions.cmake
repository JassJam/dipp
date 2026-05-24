include_guard(DIRECTORY)
include(CompilerWarnings)
include(TargetHardening)
include(TargetSanitizers)
include(StaticAnalysis)
include(StaticLinking)

#
# Apply common project options to a target
# Usage:
# target_setup_common_options(MyTarget
#   [ENABLE_EXCEPTIONS ON/OFF]                   # Override per-target exceptions
#   [ENABLE_IPO ON/OFF]                          # Enable interprocedural optimization
#   [WARNINGS_AS_ERRORS ON/OFF]                  # Override warnings as errors setting
#   [ENABLE_SANITIZER_ADDRESS ON/OFF]            # Override address sanitizer setting
#   [ENABLE_SANITIZER_LEAK ON/OFF]               # Override leak sanitizer setting
#   [ENABLE_SANITIZER_UNDEFINED_BEHAVIOR ON/OFF] # Override UB sanitizer setting
#   [ENABLE_SANITIZER_THREAD ON/OFF]             # Override thread sanitizer setting
#   [ENABLE_SANITIZER_MEMORY ON/OFF]             # Override memory sanitizer setting
#   [ENABLE_HARDENING ON/OFF]                    # Override hardening setting
#   [ENABLE_CLANG_TIDY ON/OFF]                   # Override clang-tidy setting
#   [ENABLE_CPPCHECK ON/OFF]                     # Override cppcheck setting
# )
#
function(target_setup_common_options TARGET_NAME)
    set(oneValueArgs
            ENABLE_EXCEPTIONS
            ENABLE_IPO
            WARNINGS_AS_ERRORS
            ENABLE_SANITIZER_ADDRESS
            ENABLE_SANITIZER_LEAK
            ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
            ENABLE_SANITIZER_THREAD
            ENABLE_SANITIZER_MEMORY
            ENABLE_HARDENING
            ENABLE_CLANG_TIDY
            ENABLE_CPPCHECK
            ENABLE_PCH
            ENABLE_UNITY_BUILD
    )

    cmake_parse_arguments(
            ARG
            ""
            "${oneValueArgs}"
            ""
            ${ARGN}
    )

    #
    
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "Target ${TARGET_NAME} does not exist")
    endif ()

    # Configure exceptions (per-target override or use global setting)
    set(ENABLE_EXCEPTIONS_VALUE ${ENABLE_GLOBAL_EXCEPTIONS})
    if (DEFINED ARG_ENABLE_EXCEPTIONS)
        set(ENABLE_EXCEPTIONS_VALUE ${ARG_ENABLE_EXCEPTIONS})
    endif ()

    if (ENABLE_EXCEPTIONS_VALUE)
        include(TargetExceptions)
        target_configure_exceptions(${TARGET_NAME} ${ENABLE_EXCEPTIONS_VALUE})
    endif ()

    # Configure IPO (per-target override or use global setting)
    set(ENABLE_IPO_VALUE ${ENABLE_GLOBAL_IPO})
    if (DEFINED ARG_ENABLE_IPO)
        set(ENABLE_IPO_VALUE ${ARG_ENABLE_IPO})
    endif ()

    if (ENABLE_IPO_VALUE)
        include(EnableInterproceduralOptimization)
        target_enable_interprocedural_optimization(${TARGET_NAME})
    endif ()

    # Configure compiler warnings (per-target override or use global setting)
    set(WARNINGS_AS_ERRORS_VALUE ${ENABLE_GLOBAL_WARNINGS_AS_ERRORS})
    if (DEFINED ARG_WARNINGS_AS_ERRORS)
        set(WARNINGS_AS_ERRORS_VALUE ${ARG_WARNINGS_AS_ERRORS})
    endif ()

    target_add_compiler_warnings(
            ${TARGET_NAME} PRIVATE
            WARNINGS_AS_ERRORS ${WARNINGS_AS_ERRORS_VALUE}
    )

    # Configure sanitizers (per-target override or use global settings)
    set(ENABLE_SANITIZER_ADDRESS_VALUE ${ENABLE_SANITIZER_ADDRESS})
    set(ENABLE_SANITIZER_LEAK_VALUE ${ENABLE_SANITIZER_LEAK})
    set(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR_VALUE ${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR})
    set(ENABLE_SANITIZER_THREAD_VALUE ${ENABLE_SANITIZER_THREAD})
    set(ENABLE_SANITIZER_MEMORY_VALUE ${ENABLE_SANITIZER_MEMORY})

    if (DEFINED ARG_ENABLE_SANITIZER_ADDRESS)
        set(ENABLE_SANITIZER_ADDRESS_VALUE ${ARG_ENABLE_SANITIZER_ADDRESS})
    endif ()
    if (DEFINED ARG_ENABLE_SANITIZER_LEAK)
        set(ENABLE_SANITIZER_LEAK_VALUE ${ARG_ENABLE_SANITIZER_LEAK})
    endif ()
    if (DEFINED ARG_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
        set(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR_VALUE ${ARG_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR})
    endif ()
    if (DEFINED ARG_ENABLE_SANITIZER_THREAD)
        set(ENABLE_SANITIZER_THREAD_VALUE ${ARG_ENABLE_SANITIZER_THREAD})
    endif ()
    if (DEFINED ARG_ENABLE_SANITIZER_MEMORY)
        set(ENABLE_SANITIZER_MEMORY_VALUE ${ARG_ENABLE_SANITIZER_MEMORY})
    endif ()

    set(SANITIZER_ARGS "")
    if (ENABLE_SANITIZER_ADDRESS_VALUE)
        list(APPEND SANITIZER_ARGS "ENABLE_SANITIZER_ADDRESS")
    endif ()
    if (ENABLE_SANITIZER_LEAK_VALUE)
        list(APPEND SANITIZER_ARGS "ENABLE_SANITIZER_LEAK")
    endif ()
    if (ENABLE_SANITIZER_UNDEFINED_BEHAVIOR_VALUE)
        list(APPEND SANITIZER_ARGS "ENABLE_SANITIZER_UNDEFINED_BEHAVIOR")
    endif ()
    if (ENABLE_SANITIZER_THREAD_VALUE)
        list(APPEND SANITIZER_ARGS "ENABLE_SANITIZER_THREAD")
    endif ()
    if (ENABLE_SANITIZER_MEMORY_VALUE)
        list(APPEND SANITIZER_ARGS "ENABLE_SANITIZER_MEMORY")
    endif ()

    if (NOT "${SANITIZER_ARGS}" STREQUAL "")
        include(TargetSanitizers)
        target_enable_sanitizers(${TARGET_NAME} ${SANITIZER_ARGS})
    endif ()

    # Configure hardening (per-target override or use global setting)
    set(ENABLE_HARDENING_VALUE ${ENABLE_GLOBAL_HARDENING})
    if (DEFINED ARG_ENABLE_HARDENING)
        set(ENABLE_HARDENING_VALUE ${ARG_ENABLE_HARDENING})
    endif ()

    if (ENABLE_HARDENING_VALUE)
        include(TargetHardening)
        target_enable_hardening(${TARGET_NAME} PRIVATE)
    endif ()

    # Configure static analysis (per-target override or use global settings)
    set(ENABLE_CLANG_TIDY_VALUE ${ENABLE_GLOBAL_STATIC_ANALYSIS})
    if (DEFINED ARG_ENABLE_CLANG_TIDY)
        set(ENABLE_CLANG_TIDY_VALUE ${ARG_ENABLE_CLANG_TIDY})
    endif ()
    if (DEFINED ARG_ENABLE_CPPCHECK)
        set(ENABLE_CPPCHECK_VALUE ${ARG_ENABLE_CPPCHECK})
    endif ()

    if (ENABLE_CLANG_TIDY_VALUE OR ENABLE_CPPCHECK_VALUE)
        include(StaticAnalysis)
        set(STATIC_ANALYSIS_ARGS)
        if (ENABLE_CLANG_TIDY_VALUE)
            list(APPEND STATIC_ANALYSIS_ARGS ENABLE_CLANG_TIDY)
        endif ()
        if (ENABLE_CPPCHECK_VALUE)
            list(APPEND STATIC_ANALYSIS_ARGS ENABLE_CPPCHECK)
        endif ()
        if (ENABLE_EXCEPTIONS_VALUE)
            list(APPEND STATIC_ANALYSIS_ARGS ENABLE_EXCEPTIONS)
        endif ()

        target_enable_static_analysis(
                ${TARGET_NAME}
                ${STATIC_ANALYSIS_ARGS}
        )
    endif ()

    # Enable static linking if needed
    if (ENABLE_STATIC_RUNTIME)
        include(StaticLinking)
        targets_enable_static_linking(
                TARGETS ${TARGET_NAME}
        )
    endif ()

    # Enable unity build if needed
    if (ARG_ENABLE_UNITY_BUILD)
        set_target_properties(
                ${TARGET_NAME}
                PROPERTIES
                UNITY_BUILD ${ARG_ENABLE_UNITY_BUILD}
        )
    endif ()
endfunction()
