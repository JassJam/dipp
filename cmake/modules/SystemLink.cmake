include_guard(DIRECTORY)

#
# Include a system directory (which suppresses its warnings).
#
function(target_include_system_directories TARGET_NAME)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_include_system_directories() called with invalid TARGET_NAME: ${TARGET_NAME}")
    endif ()

    set(multiValueArgs INTERFACE PUBLIC PRIVATE)
    cmake_parse_arguments(
            ARG
            ""
            ""
            "${multiValueArgs}"
            ${ARGN})

    foreach (scope IN ITEMS CMAKE_TARGET_SCOPE_TYPES)
        foreach (lib_include_dirs IN LISTS ARG_${scope})
            if (${scope} STREQUAL "INTERFACE" OR ${scope} STREQUAL "PUBLIC")
                target_include_directories(
                        ${TARGET_NAME}
                        SYSTEM
                        ${scope}
                        "$<BUILD_INTERFACE:${lib_include_dirs}>"
                        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/$<TARGET_NAME:${TARGET_NAME}>>")
            else ()
                target_include_directories(
                        ${TARGET_NAME}
                        SYSTEM
                        ${scope}
                        ${lib_include_dirs})
            endif ()
        endforeach ()
    endforeach ()

endfunction()

#
# Include the directories of a library target as system directories (which suppresses their warnings).
#
function(target_include_system_library TARGET_NAME SCOPE_NAME LIB_NAME)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_include_system_directories() called with invalid TARGET_NAME: ${TARGET_NAME}")
    endif ()

    if (NOT ${SCOPE_NAME} IN_LIST CMAKE_TARGET_SCOPE_TYPES)
        message(FATAL_ERROR "target_add_compiler_warnings() called with invalid SCOPE: ${SCOPE_NAME}")
    endif ()

    # check if this is a target
    if (TARGET ${lib})
        get_target_property(lib_include_dirs ${LIB_NAME} INTERFACE_INCLUDE_DIRECTORIES)
        if (lib_include_dirs)
            target_include_system_directories(${TARGET_NAME} ${SCOPE_NAME} ${lib_include_dirs})
        else ()
            message(TRACE "${LIB_NAME} library does not have the INTERFACE_INCLUDE_DIRECTORIES property.")
        endif ()
    endif ()
endfunction()

#
# Link a library target as a system library (which suppresses its warnings).
#
function(target_link_system_library TARGET_NAME SCOPE_NAME LIB_NAME)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_include_system_directories() called with invalid TARGET_NAME: ${TARGET_NAME}")
    endif ()

    if (NOT ${SCOPE_NAME} IN_LIST CMAKE_TARGET_SCOPE_TYPES)
        message(FATAL_ERROR "target_add_compiler_warnings() called with invalid SCOPE: ${SCOPE_NAME}")
    endif ()

    # Include the directories in the library
    target_include_system_library(${TARGET_NAME} ${SCOPE_NAME} ${LIB_NAME})

    # Link the library
    target_link_libraries(${TARGET_NAME} ${SCOPE_NAME} ${LIB_NAME})
endfunction()

#
# Link multiple library targets as system libraries (which suppresses their warnings).
#
function(target_link_system_libraries TARGET_NAME)
    set(multiValueArgs INTERFACE PUBLIC PRIVATE)
    cmake_parse_arguments(
            ARG
            ""
            ""
            "${multiValueArgs}"
            ${ARGN})

    foreach (scope IN ITEMS CMAKE_TARGET_SCOPE_TYPES)
        foreach (lib IN LISTS ARG_${scope})
            target_link_system_library(${TARGET_NAME} ${scope} ${lib})
        endforeach ()
    endforeach ()
endfunction()
