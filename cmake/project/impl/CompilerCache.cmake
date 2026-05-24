include_guard(DIRECTORY)

#
# Compiler cache support (ccache/sccache)
#

if (ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    find_program(SCCACHE_PROGRAM sccache)

    if (CCACHE_PROGRAM)
        message(STATUS "** ccache found: ${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    elseif (SCCACHE_PROGRAM)
        message(STATUS "** sccache found: ${SCCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    else ()
        message(STATUS "** No compiler cache found (ccache/sccache)")
    endif ()
endif ()
