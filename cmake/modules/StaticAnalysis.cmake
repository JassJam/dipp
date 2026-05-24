include_guard(DIRECTORY)

#
# usage:
# target_enable_static_analysis(target_name
#   [ENABLE_CLANG_TIDY]
#   [ENABLE_CPPCHECK]
#   [ENABLE_EXCEPTIONS]
# )
#
function(target_enable_static_analysis TARGET_NAME)
    set(options
            ENABLE_CLANG_TIDY
            ENABLE_CPPCHECK
            ENABLE_EXCEPTIONS
    )

    cmake_parse_arguments(ARG "${options}" "" "" ${ARGN})

    #

    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_enable_static_analysis: Target '${TARGET_NAME}' does not exist")
    endif ()

    # clang-tidy setup
    if (ARG_ENABLE_CLANG_TIDY)
        _configure_clang_tidy(${TARGET_NAME} ${ARG_ENABLE_EXCEPTIONS})
    endif ()

    # cppcheck setup
    if (ARG_ENABLE_CPPCHECK)
        _configure_cppcheck(${TARGET_NAME})
    endif ()
endfunction()

#

# Helper function to create clang-tidy custom targets
function(_configure_clang_tidy TARGET_NAME ENABLE_EXCEPTIONS)
    _find_clang_tidy(CLANG_TIDY_EXE)
    if (NOT CLANG_TIDY_EXE)
        message(STATUS "clang-tidy requested but not found")
        return()
    endif ()

    message(STATUS "** clang-tidy found: ${CLANG_TIDY_EXE}")
    _add_clang_tidy_custom_target(${TARGET_NAME} ${ENABLE_EXCEPTIONS} ${CLANG_TIDY_EXE})
endfunction()

# Helper function to create clang-tidy custom targets
function(_add_clang_tidy_custom_target TARGET_NAME ENABLE_EXCEPTIONS CLANG_TIDY_EXE)
    if (NOT CLANG_TIDY_EXE)
        return()
    endif ()

    # Build clang-tidy arguments
    set(CXX_CLANG_TIDY_ARGS "${CLANG_TIDY_EXE}")
    list(APPEND CXX_CLANG_TIDY_ARGS "--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy")
    file(RELATIVE_PATH BINARY_DIR_RELATIVE "${PROJECT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

    if (WIN32)
        list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-dll-attribute-on-redeclaration")
        list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-inconsistent-dllimport")
    endif ()

    get_current_compiler(CURRENT_COMPILER)
    list(APPEND CXX_CLANG_TIDY_ARGS "--use-color")

    if (CURRENT_COMPILER MATCHES "MSVC")
        if (${ENABLE_EXCEPTIONS})
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=/EHsc")
        else ()
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=/EHs-c-")
        endif ()
    elseif (CURRENT_COMPILER MATCHES "CLANG.*|GCC|EMSCRIPTEN")
        if (${ENABLE_EXCEPTIONS})
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-fexceptions")
        else ()
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-fno-exceptions")
        endif ()

        list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-unused-command-line-argument")
        list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-unknown-argument")

        # When using GCC compiler, ignore GCC-specific warning flags that clang-tidy doesn't understand
        if (CURRENT_COMPILER MATCHES "GCC")
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-error=unknown-warning-option")
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-duplicated-branches")
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-duplicated-cond")
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-logical-op")
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg=-Wno-useless-cast")
            # Tell clang-tidy to use GCC driver mode for compatibility
            list(APPEND CXX_CLANG_TIDY_ARGS "--extra-arg-before=--driver-mode=g++")
        endif ()
    endif ()

    set_target_properties(${TARGET_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CXX_CLANG_TIDY_ARGS}"
    )

    if (MSVC)
        set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_EnableMicrosoftCodeAnalysis false)
        set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_EnableClangTidyCodeAnalysis true)
        set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_RunCodeAnalysis true)
    endif ()
endfunction()

#

# Helper function to configure cppcheck for a target
function(_configure_cppcheck TARGET_NAME)
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if (CPPCHECK_EXE)
        message(STATUS "** cppcheck found: ${CPPCHECK_EXE}")
        set(CXX_CPPCHECK_ARGS "${CPPCHECK_EXE}")
        list(APPEND CXX_CPPCHECK_ARGS "--enable=warning,performance,portability,information,missingInclude")
        list(APPEND CXX_CPPCHECK_ARGS "--std=c++${CMAKE_CXX_STANDARD}")
        list(APPEND CXX_CPPCHECK_ARGS "--template=gcc")
        list(APPEND CXX_CPPCHECK_ARGS "--verbose")
        list(APPEND CXX_CPPCHECK_ARGS "--quiet")
        list(APPEND CXX_CPPCHECK_ARGS "--error-exitcode=1")

        set_target_properties(${TARGET_NAME} PROPERTIES
                CXX_CPPCHECK "${CXX_CPPCHECK_ARGS}"
        )
        message(STATUS "** cppcheck enabled for target: ${TARGET_NAME}")
    else ()
        message(STATUS "cppcheck requested but not found")
    endif ()
endfunction()

#

function(lint_source_file SOURCE_FILE)

    set(oneValueArgs
            ENABLE_CLANG_TIDY
            ENABLE_CPPCHECK
            ENABLE_EXCEPTIONS
    )
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    #

    if (NOT ARG_ENABLE_EXCEPTIONS)
        set(ARG_ENABLE_EXCEPTIONS OFF)
    endif ()

    if (NOT IS_ABSOLUTE "${SOURCE_FILE}")
        get_filename_component(SOURCE_FILE "${SOURCE_FILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif ()

    if (NOT EXISTS "${SOURCE_FILE}")
        message(FATAL_ERROR "lint_source_file: Source file '${SOURCE_FILE}' does not exist")
    endif ()

    string(FIND "${SOURCE_FILE}" "${CMAKE_BINARY_DIR}" is_in_binary_dir)
    if (NOT is_in_binary_dir EQUAL -1)
        message(STATUS "** Skipping linting for generated file: ${SOURCE_FILE}")
        return()
    endif ()

    if (ARG_ENABLE_CLANG_TIDY)
        _find_clang_tidy(CLANG_TIDY_EXE)
        if (NOT CLANG_TIDY_EXE)
            message(STATUS "clang-tidy requested but not found")
            return()
        endif ()

        _build_clang_tidy_command(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "${SOURCE_FILE}" ${ARG_ENABLE_EXCEPTIONS})

        execute_process(COMMAND ${CLANG_TIDY_COMMAND}
                RESULT_VARIABLE CLANG_TIDY_RESULT
                OUTPUT_VARIABLE CLANG_TIDY_OUTPUT
                ERROR_VARIABLE CLANG_TIDY_ERROR
        )

        if (CLANG_TIDY_RESULT EQUAL 0)
            message(STATUS "** clang-tidy passed for: ${SOURCE_FILE}")
        else ()
            message(STATUS "** clang-tidy found issues in: ${SOURCE_FILE}")
            message(WARNING "${CLANG_TIDY_RESULT}")
            message(WARNING "${CLANG_TIDY_OUTPUT}")
            message(WARNING "${CLANG_TIDY_ERROR}")
        endif ()
    endif ()

endfunction()

#

function(_find_clang_tidy OUT_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

    # If not found in PATH, try to find it using vswhere (Windows/Visual Studio)
    if (NOT CLANG_TIDY_EXE AND WIN32)
        find_program(VSWHERE_EXE NAMES "vswhere"
                PATHS "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer"
                "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer")

        if (VSWHERE_EXE)
            # First try to find VS with LLVM component specifically
            execute_process(
                    COMMAND "${VSWHERE_EXE}" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath
                    OUTPUT_VARIABLE VS_INSTALLATION_PATH
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
            )

            # If that fails, try with any VS installation that has VC tools
            if (NOT VS_INSTALLATION_PATH)
                execute_process(
                        COMMAND "${VSWHERE_EXE}" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
                        OUTPUT_VARIABLE VS_INSTALLATION_PATH
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_QUIET
                )
            endif ()

            # If still not found, try any VS installation (including Preview)
            if (NOT VS_INSTALLATION_PATH)
                execute_process(
                        COMMAND "${VSWHERE_EXE}" -all -products * -property installationPath
                        OUTPUT_VARIABLE VS_INSTALLATION_PATHS
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_QUIET
                )
                # Take the first line (latest installation)
                string(REGEX REPLACE "\n.*" "" VS_INSTALLATION_PATH "${VS_INSTALLATION_PATHS}")
            endif ()

            if (VS_INSTALLATION_PATH)
                # Try to find clang-tidy in LLVM tools
                find_program(CLANG_TIDY_EXE NAMES "clang-tidy"
                        PATHS "${VS_INSTALLATION_PATH}/VC/Tools/Llvm/x64/bin"
                        "${VS_INSTALLATION_PATH}/VC/Tools/Llvm/bin"
                        NO_DEFAULT_PATH
                )

                if (CLANG_TIDY_EXE)
                    message(STATUS "** clang-tidy found via vswhere: ${CLANG_TIDY_EXE}")
                else ()
                    # If clang-tidy not found, check if we can enable MSVC static analysis
                    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                        message(STATUS "** clang-tidy not found, but MSVC compiler detected")
                        message(STATUS "** Consider using MSVC's built-in /analyze flag or installing LLVM tools")
                        # Set a flag to indicate MSVC static analysis could be used instead
                        set(MSVC_STATIC_ANALYSIS_AVAILABLE TRUE PARENT_SCOPE)
                    endif ()
                endif ()
            endif ()
        endif ()
    endif ()
endfunction()

function(_build_clang_tidy_command OUT_CLANG_TIDY_COMMAND CLANG_TIDY_EXE SOURCE_FILE ENABLE_EXCEPTIONS)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}")
    list(APPEND CLANG_TIDY_COMMAND "--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy")
    list(APPEND CLANG_TIDY_COMMAND "--header-filter=(.*\/(out|build\/).*)")

    if (WIN32)
        list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-dll-attribute-on-redeclaration")
        list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-inconsistent-dllimport")
    endif ()

    get_current_compiler(CURRENT_COMPILER)
    list(APPEND CLANG_TIDY_COMMAND "--use-color")

    if (CURRENT_COMPILER MATCHES "MSVC")
        if (${ENABLE_EXCEPTIONS})
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=/EHsc")
        else ()
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=/EHs-c-")
        endif ()
    elseif (CURRENT_COMPILER MATCHES "CLANG.*|GCC|EMSCRIPTEN")
        if (${ENABLE_EXCEPTIONS})
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-fexceptions")
        else ()
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-fno-exceptions")
        endif ()

        list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-unused-command-line-argument")
        list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-unknown-argument")

        # When using GCC compiler, ignore GCC-specific warning flags that clang-tidy doesn't understand
        if (CURRENT_COMPILER MATCHES "GCC")
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-error=unknown-warning-option")
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-duplicated-branches")
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-duplicated-cond")
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-logical-op")
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg=-Wno-useless-cast")
            # Tell clang-tidy to use GCC driver mode for compatibility
            list(APPEND CLANG_TIDY_COMMAND "--extra-arg-before=--driver-mode=g++")
        endif ()
    endif ()
    list(APPEND CLANG_TIDY_COMMAND "${SOURCE_FILE}")
    set(${OUT_CLANG_TIDY_COMMAND} "${CLANG_TIDY_COMMAND}" PARENT_SCOPE)

endfunction()
