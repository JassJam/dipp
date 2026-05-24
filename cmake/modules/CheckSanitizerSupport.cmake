include_guard(DIRECTORY)

#
# Check sanitizers support across different compilers
#
function(check_sanitizers_support
        SUPPORTS_UBSAN
        SUPPORTS_ASAN
)
    # UBSan support
    if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR
            CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*")
            AND NOT WIN32)
        set(${SUPPORTS_UBSAN} ON PARENT_SCOPE)
    else ()
        set(${SUPPORTS_UBSAN} OFF PARENT_SCOPE)
    endif ()

    # ASan support - disabled on Windows+GCC
    if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*") OR
    (CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*" AND NOT WIN32) OR
    (CMAKE_CXX_COMPILER_ID MATCHES "MSVC"))
        set(${SUPPORTS_ASAN} ON PARENT_SCOPE)
    else ()
        set(${SUPPORTS_ASAN} OFF PARENT_SCOPE)
    endif ()
endfunction()