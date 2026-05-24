include_guard(DIRECTORY)

include(CheckSanitizerSupport)
include(CMakeDependentOption)

# Check what sanitizers are supported
check_sanitizers_support(SUPPORTS_UBSAN SUPPORTS_ASAN)

#

# === CACHE CONFIGURATION OPTIONS ===
set(ENABLE_CCACHE ON CACHE BOOL "Enable ccache for faster rebuilds")

mark_as_advanced(ENABLE_CCACHE)

# === PACKAGE MANAGEMENT OPTIONS ===
set(PACKAGE_MANAGERS "CPM" CACHE STRING "Package managers to enable (semicolon-separated list: CPM, XMake)")

# === MAIN CONFIGURATION OPTIONS ===
option(DEV_MODE "Enable development mode (all quality tools)" ON)
option(RELEASE_MODE "Enable release optimizations" OFF)

cmake_dependent_option(
        DEV_MODE
        "Enable development mode (all quality tools)"
        ON "NOT RELEASE_MODE" OFF
)
cmake_dependent_option(
        RELEASE_MODE
        "Enable release optimizations"
        OFF "NOT DEV_MODE" ON
)

# === GLOBAL OPTIONS ===
set(ENABLE_GLOBAL_EXCEPTIONS "ON" CACHE STRING "Enable global exception handling")
set(ENABLE_GLOBAL_WARNINGS_AS_ERRORS "${DEV_MODE}" CACHE STRING "Enable global warnings as errors")
set(ENABLE_GLOBAL_SANITIZERS "${DEV_MODE}" CACHE STRING "Enable global sanitizers")
set(ENABLE_GLOBAL_HARDENING "${DEV_MODE}" CACHE STRING "Enable global hardening")
set(ENABLE_GLOBAL_STATIC_ANALYSIS "${DEV_MODE}" CACHE STRING "Enable global static analysis")

# === SANITIZER OPTIONS ===
if (DEV_MODE OR ENABLE_GLOBAL_SANITIZERS)
    set(DEFAULT_ASAN ${SUPPORTS_ASAN})
    set(DEFAULT_UBSAN ${SUPPORTS_UBSAN})
else ()
    set(DEFAULT_ASAN OFF)
    set(DEFAULT_UBSAN OFF)
endif ()

option(ENABLE_ASAN "Enable address sanitizer (detects memory errors)" ${DEFAULT_ASAN})
option(ENABLE_LSAN "Enable leak sanitizer (detects memory leaks)" OFF)
option(ENABLE_UBSAN "Enable undefined behavior sanitizer" ${DEFAULT_UBSAN})
option(ENABLE_TSAN "Enable thread sanitizer (detects data races)" OFF)
option(ENABLE_MSAN "Enable memory sanitizer (detects uninitialized reads)" OFF)

# === DEBUG OPTIONS ===
option(ENABLE_EDIT_AND_CONTINUE "Enable Edit and Continue support (MSVC /ZI)" ${DEV_MODE})
option(ENABLE_DEBUG_INFO "Enable debug information generation" ${DEV_MODE})
if (NOT ENABLE_DEBUG_INFO)
    set(DEBUG_INFO_LEVEL "0" CACHE STRING "Debug information level (0-3 for GCC/Clang, ignored for MSVC)")
else ()
    set(DEBUG_INFO_LEVEL "2" CACHE STRING "Debug information level (0-3 for GCC/Clang, ignored for MSVC)")
endif ()

# === LINKING OPTIONS ===
option(ENABLE_STATIC_RUNTIME "Statically link runtime libraries for better portability" OFF)
option(ENABLE_GLOBAL_IPO "Enable global link-time optimization (LTO)" ${RELEASE_MODE})

# === EMSCRIPTEN OPTIONS ===
option(ENABLE_EMSDK_AUTO_INSTALL "Automatically install EMSDK locally if not found" ON)

# === TESTING OPTIONS ==
set(BUILD_TESTING ON CACHE BOOL "Build and enable testing")

#

# Mark advanced options
mark_as_advanced(
        ENABLE_ASAN ENABLE_LSAN ENABLE_UBSAN ENABLE_TSAN ENABLE_MSAN
        ENABLE_CLANG_TIDY ENABLE_CPPCHECK
        ENABLE_UNITY_BUILD ENABLE_PCH
        ENABLE_EMSDK_AUTO_INSTALL
        ENABLE_EXCEPTIONS
        ENABLE_EDIT_AND_CONTINUE
)

# Set up global hardening based on sanitizer settings
cmake_dependent_option(
        ENABLE_GLOBAL_HARDENING
        "Enable security hardening options (stack protection, etc.)"
        ON "ENABLE_GLOBAL_SANITIZERS OR DEV_MODE" OFF
)

# ENABLE_EDIT_AND_CONTINUE is not compatible with asan, so disable it if ASan is enabled
cmake_dependent_option(
        ENABLE_EDIT_AND_CONTINUE
        "Enable Edit&Continue for debugging (requires MSVC)"
        ON "NOT ENABLE_ASAN AND NOT ENABLE_LSAN" OFF
)

# Apply global hardening immediately if enabled
if (ENABLE_GLOBAL_HARDENING)
    include(TargetHardening)
    enable_global_hardening()
endif ()

# Apply global IPO if enabled
if (ENABLE_GLOBAL_IPO)
    include(EnableInterproceduralOptimization)
    enable_global_interprocedural_optimization()
endif ()

# Apply global sanitizers if enabled
if (ENABLE_GLOBAL_SANITIZERS)
    include(TargetSanitizers)
    enable_global_sanitizers()
endif ()

# Apply global exceptions settings
if (ENABLE_GLOBAL_EXCEPTIONS)
    include(TargetExceptions)
    configure_global_exceptions(${ENABLE_GLOBAL_EXCEPTIONS})
endif ()

# Configure static linking flags with auto-detection
if (ENABLE_STATIC_RUNTIME)
    include(StaticLinking)
    enable_static_linking()
endif ()

# Apply global debug options if enabled
if (ENABLE_EDIT_AND_CONTINUE OR ENABLE_DEBUG_INFO)
    include(TargetDebugOptions)
    enable_global_debug_options()
endif ()

# Print configuration summary
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "DEV_MODE: ${DEV_MODE}")
message(STATUS "RELEASE_MODE: ${RELEASE_MODE}")
message(STATUS "Sanitizers: ${ENABLE_GLOBAL_SANITIZERS} (ASan:${ENABLE_ASAN}, UBSan:${ENABLE_UBSAN})")
message(STATUS "Static analysis: ${ENABLE_GLOBAL_STATIC_ANALYSIS}")
message(STATUS "Debug options: Edit&Continue:${ENABLE_EDIT_AND_CONTINUE}, DebugInfo:${ENABLE_DEBUG_INFO} (level:${DEBUG_INFO_LEVEL})")
message(STATUS "Static linking: runtime:${ENABLE_STATIC_RUNTIME}")
message(STATUS "=== End of Configuration ===")