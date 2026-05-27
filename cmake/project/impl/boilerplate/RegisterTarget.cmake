include_guard(DIRECTORY)
include(${CMAKE_CURRENT_LIST_DIR}/CopySharedLibrary.cmake)

function(_register_target_common target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
            ""
            "NAMESPACE;EXPORT_SET;INSTALL_DESTINATION;CXX_STANDARD"
            "COMPILE_OPTIONS;COMPILE_DEFINITIONS;INCLUDE_DIRS;LINK_LIBS;PROPERTIES"
    )

    if (DEFINED ARG_CXX_STANDARD)
        set_target_properties(${target} PROPERTIES
                CXX_STANDARD ${ARG_CXX_STANDARD}
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
        )
    endif ()

    if (ARG_COMPILE_OPTIONS)
        target_compile_options(${target} ${ARG_COMPILE_OPTIONS})
    endif ()

    if (ARG_COMPILE_DEFINITIONS)
        target_compile_definitions(${target} ${ARG_COMPILE_DEFINITIONS})
    endif ()

    if (ARG_INCLUDE_DIRS)
        set(_vis PUBLIC)
        foreach (_inc IN LISTS ARG_INCLUDE_DIRS)
            if (_inc STREQUAL "PUBLIC" OR _inc STREQUAL "PRIVATE" OR _inc STREQUAL "INTERFACE")
                set(_vis "${_inc}")
            else ()
                if (NOT IS_ABSOLUTE "${_inc}")
                    set(_inc "${CMAKE_CURRENT_SOURCE_DIR}/${_inc}")
                endif ()
                if (_vis STREQUAL "PRIVATE")
                    target_include_directories(${target} PRIVATE
                            "$<BUILD_INTERFACE:${_inc}>"
                    )
                elseif (_vis STREQUAL "INTERFACE")
                    target_include_directories(${target} INTERFACE
                            "$<BUILD_INTERFACE:${_inc}>"
                            "$<INSTALL_INTERFACE:include>"
                    )
                else ()
                    target_include_directories(${target} PUBLIC
                            "$<BUILD_INTERFACE:${_inc}>"
                            "$<INSTALL_INTERFACE:include>"
                    )
                endif ()
            endif ()
        endforeach ()
    endif ()

    if (ARG_LINK_LIBS)
        target_link_libraries(${target} ${ARG_LINK_LIBS})
    endif ()

    if (ARG_PROPERTIES)
        set_target_properties(${target} PROPERTIES ${ARG_PROPERTIES})
    endif ()

    # Configure RPATH for shared library dependencies
    if (UNIX)
        set_target_properties(${target} PROPERTIES
                # Don't skip the full RPATH for the build tree
                SKIP_BUILD_RPATH FALSE
                # When building, don't use the install RPATH already
                BUILD_WITH_INSTALL_RPATH FALSE
                # Add the automatically determined parts of the RPATH
                # which point to directories outside the build tree to the install RPATH
                INSTALL_RPATH_USE_LINK_PATH TRUE
                # The RPATH to be used when installing - executables and libraries in same directory
                INSTALL_RPATH "$ORIGIN"
        )
    endif ()
    _copy_shared_library_dependencies_to_build_dir(${target})

    if (DEFINED ARG_EXPORT_SET)
        set(_dest "${ARG_INSTALL_DESTINATION}")
        if (NOT _dest)
            get_target_property(_type ${target} TYPE)
            if (_type STREQUAL "EXECUTABLE")
                set(_dest "bin")
            else ()
                set(_dest "lib")
            endif ()
        endif ()

        set(_ns "")
        if (DEFINED ARG_NAMESPACE)
            set(_ns NAMESPACE "${ARG_NAMESPACE}::")
        endif ()

        install(TARGETS ${target}
                EXPORT ${ARG_EXPORT_SET}
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION ${_dest}
                ARCHIVE DESTINATION ${_dest}
                CXX_MODULES_BMI DESTINATION lib/bmi
                FILE_SET HEADERS DESTINATION include
                FILE_SET CXX_MODULES DESTINATION include/modules
        )

        get_property(_exported_sets GLOBAL PROPERTY _REGISTER_EXPORTED_SETS)
        if (NOT ARG_EXPORT_SET IN_LIST _exported_sets)
            list(APPEND _exported_sets ${ARG_EXPORT_SET})
            set_property(GLOBAL PROPERTY _REGISTER_EXPORTED_SETS "${_exported_sets}")

            install(EXPORT ${ARG_EXPORT_SET}
                    FILE "${ARG_EXPORT_SET}Targets.cmake"
                    ${_ns}
                    DESTINATION "lib/cmake/${ARG_EXPORT_SET}"
            )
        endif ()
    endif ()
endfunction()


# _register_forward_quality_opts(<target>)
# Reads ARG_ENABLE_* from the calling scope and forwards to
# target_setup_common_options() if that command exists.
macro(_register_forward_quality_opts target)
    set(_quality_args)
    foreach (_q
            ENABLE_EXCEPTIONS ENABLE_IPO WARNINGS_AS_ERRORS
            ENABLE_SANITIZER_ADDRESS ENABLE_SANITIZER_LEAK
            ENABLE_SANITIZER_UNDEFINED_BEHAVIOR ENABLE_SANITIZER_THREAD
            ENABLE_SANITIZER_MEMORY ENABLE_HARDENING
            ENABLE_CLANG_TIDY ENABLE_CPPCHECK
    )
        if (DEFINED ARG_${_q})
            list(APPEND _quality_args ${_q} ${ARG_${_q}})
        endif ()
    endforeach ()

    if (_quality_args AND COMMAND target_setup_common_options)
        target_setup_common_options(${target} ${_quality_args})
    endif ()
    unset(_quality_args)
    unset(_q)
endmacro()


# register_header_only_library(<name>
#     [HEADERS            <file> …]
#     [INCLUDE_DIRS       <dir>  …]
#     [LINK_LIBS          <tgt>  …]
#     [COMPILE_DEFINITIONS <def> …]
#     [PROPERTIES         <key val> …]
#     [NAMESPACE          <ns>]
#     [EXPORT_SET         <set>]
#     [INSTALL_DESTINATION <dir>]
# )
function(register_header_only_library name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
            ""
            "NAMESPACE;EXPORT_SET;INSTALL_DESTINATION;CXX_STANDARD;HEADER_BASE_DIR"
            "HEADERS;INCLUDE_DIRS;LINK_LIBS;COMPILE_DEFINITIONS;PROPERTIES"
    )

    add_library(${name} INTERFACE)
    add_library(${name}::${name} ALIAS ${name})

    if (NOT ARG_HEADER_BASE_DIR)
        set(ARG_HEADER_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif ()
    if (ARG_HEADERS)
        target_sources(${name} INTERFACE
                FILE_SET HEADERS
                BASE_DIRS "${ARG_HEADER_BASE_DIR}"
                FILES ${ARG_HEADERS}
        )
    endif ()

    if (ARG_INCLUDE_DIRS)
        foreach (_inc IN LISTS ARG_INCLUDE_DIRS)
            # INTERFACE libraries only support INTERFACE scope
            if (_inc STREQUAL "PUBLIC" OR _inc STREQUAL "PRIVATE" OR _inc STREQUAL "INTERFACE")
                continue()
            endif ()
            if (NOT IS_ABSOLUTE "${_inc}")
                set(_inc "${CMAKE_CURRENT_SOURCE_DIR}/${_inc}")
            endif ()
            target_include_directories(${name} INTERFACE
                    "$<BUILD_INTERFACE:${_inc}>"
                    "$<INSTALL_INTERFACE:include>"
            )
        endforeach ()
    endif ()

    if (ARG_LINK_LIBS)
        target_link_libraries(${name} INTERFACE ${ARG_LINK_LIBS})
    endif ()

    if (ARG_COMPILE_DEFINITIONS)
        target_compile_definitions(${name} INTERFACE ${ARG_COMPILE_DEFINITIONS})
    endif ()

    if (ARG_PROPERTIES)
        set_target_properties(${name} PROPERTIES ${ARG_PROPERTIES})
    endif ()

    set(_forward)
    foreach (_kw NAMESPACE EXPORT_SET INSTALL_DESTINATION CXX_STANDARD)
        if (DEFINED ARG_${_kw})
            list(APPEND _forward ${_kw} "${ARG_${_kw}}")
        endif ()
    endforeach ()

    _register_target_common(${name} ${_forward})
endfunction()


# register_library(<name>
#     [STATIC | SHARED]
#     [SOURCES            <file> …]
#     [HEADERS            <file> …]
#     [CXX_MODULES        <file> …]
#     [INCLUDE_DIRS       <dir>  …]
#     [LINK_LIBS          <tgt>  …]
#     [CXX_STANDARD       <std>]       default: 23 with modules, else 17
#     [NAMESPACE          <ns>]
#     [EXPORT_SET         <set>]
#     [INSTALL_DESTINATION <dir>]
#     [COMPILE_OPTIONS     <opt> …]
#     [COMPILE_DEFINITIONS <def> …]
#     [PROPERTIES         <key val> …]
#     [EXPORT_HEADER      <relative/path/export.hpp>]
#     [EXPORT_MACRO_NAME  <MACRO>]
#     [ENABLE_EXCEPTIONS ON|OFF]
#     [ENABLE_IPO ON|OFF]
#     [WARNINGS_AS_ERRORS ON|OFF]
#     [ENABLE_SANITIZER_ADDRESS ON|OFF]
#     [ENABLE_SANITIZER_LEAK ON|OFF]
#     [ENABLE_SANITIZER_UNDEFINED_BEHAVIOR ON|OFF]
#     [ENABLE_SANITIZER_THREAD ON|OFF]
#     [ENABLE_SANITIZER_MEMORY ON|OFF]
#     [ENABLE_HARDENING ON|OFF]
#     [ENABLE_CLANG_TIDY ON|OFF]
#     [ENABLE_CPPCHECK ON|OFF]
# )
function(register_library name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
            "STATIC;SHARED"
            "NAMESPACE;EXPORT_SET;INSTALL_DESTINATION;CXX_STANDARD;EXPORT_HEADER;EXPORT_MACRO_NAME;HEADER_BASE_DIR;
         ENABLE_EXCEPTIONS;ENABLE_IPO;WARNINGS_AS_ERRORS;
         ENABLE_SANITIZER_ADDRESS;ENABLE_SANITIZER_LEAK;
         ENABLE_SANITIZER_UNDEFINED_BEHAVIOR;ENABLE_SANITIZER_THREAD;
         ENABLE_SANITIZER_MEMORY;ENABLE_HARDENING;
         ENABLE_CLANG_TIDY;ENABLE_CPPCHECK"
            "SOURCES;HEADERS;CXX_MODULES;INCLUDE_DIRS;LINK_LIBS;COMPILE_OPTIONS;COMPILE_DEFINITIONS;PROPERTIES"
    )

    if (ARG_STATIC)
        set(_linkage STATIC)
    elseif (ARG_SHARED)
        set(_linkage SHARED)
    else ()
        set(_linkage "")
    endif ()

    add_library(${name} ${_linkage})
    add_library(${name}::${name} ALIAS ${name})

    if (ARG_SOURCES)
        target_sources(${name} PRIVATE ${ARG_SOURCES})
    endif ()

    if (NOT ARG_HEADER_BASE_DIR)
        set(ARG_HEADER_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif ()
    if (ARG_HEADERS)
        target_sources(${name} PUBLIC
                FILE_SET HEADERS
                BASE_DIRS "${ARG_HEADER_BASE_DIR}"
                FILES ${ARG_HEADERS}
        )
    endif ()

    if (ARG_CXX_MODULES)
        if (NOT DEFINED ARG_CXX_STANDARD)
            set(ARG_CXX_STANDARD 23)
        endif ()
        target_sources(${name} PUBLIC
                FILE_SET CXX_MODULES
                BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
                FILES ${ARG_CXX_MODULES}
        )
    endif ()

    if (NOT DEFINED ARG_CXX_STANDARD)
        if (DEFINED CMAKE_CXX_STANDARD)
            set(ARG_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        else ()
            set(ARG_CXX_STANDARD 17)
        endif ()
    endif ()

    target_include_directories(${name} PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
            "$<INSTALL_INTERFACE:include>"
    )

    if (DEFINED ARG_EXPORT_HEADER)
        include(GenerateExportHeader)

        set(_export_file "${CMAKE_CURRENT_BINARY_DIR}/${ARG_EXPORT_HEADER}")

        set(_geh_extra)
        if (DEFINED ARG_EXPORT_MACRO_NAME)
            list(APPEND _geh_extra EXPORT_MACRO_NAME "${ARG_EXPORT_MACRO_NAME}")
        endif ()

        generate_export_header(${name}
                EXPORT_FILE_NAME "${_export_file}"
                ${_geh_extra}
        )

        target_sources(${name} PUBLIC
                FILE_SET HEADERS
                BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}"
                FILES "${_export_file}"
        )

        target_include_directories(${name} PUBLIC
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
                "$<INSTALL_INTERFACE:include>"
        )

        get_target_property(_lib_type ${name} TYPE)
        if (_lib_type STREQUAL "STATIC_LIBRARY")
            string(TOUPPER "${name}" _upper)
            target_compile_definitions(${name} PUBLIC ${_upper}_STATIC_DEFINE)
        endif ()
    endif ()

    set(_forward CXX_STANDARD ${ARG_CXX_STANDARD})
    foreach (_kw NAMESPACE EXPORT_SET INSTALL_DESTINATION)
        if (DEFINED ARG_${_kw})
            list(APPEND _forward ${_kw} "${ARG_${_kw}}")
        endif ()
    endforeach ()
    foreach (_mv INCLUDE_DIRS LINK_LIBS COMPILE_OPTIONS COMPILE_DEFINITIONS PROPERTIES)
        if (ARG_${_mv})
            list(APPEND _forward ${_mv} ${ARG_${_mv}})
        endif ()
    endforeach ()

    _register_target_common(${name} ${_forward})
    _register_forward_quality_opts(${name})
endfunction()


# register_executable(<name>
#     [SOURCES            <file> …]
#     [HEADERS            <file> …]
#     [CXX_MODULES        <file> …]
#     [INCLUDE_DIRS       <dir>  …]
#     [LINK_LIBS          <tgt>  …]
#     [CXX_STANDARD       <std>]
#     [NAMESPACE          <ns>]
#     [EXPORT_SET         <set>]
#     [INSTALL_DESTINATION <dir>]      default: bin
#     [COMPILE_OPTIONS     <opt> …]
#     [COMPILE_DEFINITIONS <def> …]
#     [PROPERTIES         <key val> …]
#     [ENABLE_EXCEPTIONS ON|OFF]
#     [ENABLE_IPO ON|OFF]
#     [WARNINGS_AS_ERRORS ON|OFF]
#     [ENABLE_SANITIZER_ADDRESS ON|OFF]
#     [ENABLE_SANITIZER_LEAK ON|OFF]
#     [ENABLE_SANITIZER_UNDEFINED_BEHAVIOR ON|OFF]
#     [ENABLE_SANITIZER_THREAD ON|OFF]
#     [ENABLE_SANITIZER_MEMORY ON|OFF]
#     [ENABLE_HARDENING ON|OFF]
#     [ENABLE_CLANG_TIDY ON|OFF]
#     [ENABLE_CPPCHECK ON|OFF]
# )
function(register_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
            ""
            "NAMESPACE;EXPORT_SET;INSTALL_DESTINATION;CXX_STANDARD;HEADER_BASE_DIR;
         ENABLE_EXCEPTIONS;ENABLE_IPO;WARNINGS_AS_ERRORS;
         ENABLE_SANITIZER_ADDRESS;ENABLE_SANITIZER_LEAK;
         ENABLE_SANITIZER_UNDEFINED_BEHAVIOR;ENABLE_SANITIZER_THREAD;
         ENABLE_SANITIZER_MEMORY;ENABLE_HARDENING;
         ENABLE_CLANG_TIDY;ENABLE_CPPCHECK"
            "SOURCES;HEADERS;CXX_MODULES;INCLUDE_DIRS;LINK_LIBS;COMPILE_OPTIONS;COMPILE_DEFINITIONS;PROPERTIES"
    )

    add_executable(${name})

    if (ARG_SOURCES)
        target_sources(${name} PRIVATE ${ARG_SOURCES})
    endif ()

    if (NOT ARG_HEADER_BASE_DIR)
        set(ARG_HEADER_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif ()
    if (ARG_HEADERS)
        target_sources(${name} PRIVATE
                FILE_SET HEADERS
                BASE_DIRS "${ARG_HEADER_BASE_DIR}"
                FILES ${ARG_HEADERS}
        )
    endif ()

    if (ARG_CXX_MODULES)
        if (NOT DEFINED ARG_CXX_STANDARD)
            set(ARG_CXX_STANDARD 23)
        endif ()
        target_sources(${name} PRIVATE
                FILE_SET CXX_MODULES
                BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
                FILES ${ARG_CXX_MODULES}
        )
    endif ()

    if (NOT DEFINED ARG_CXX_STANDARD)
        if (DEFINED CMAKE_CXX_STANDARD)
            set(ARG_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        else ()
            set(ARG_CXX_STANDARD 17)
        endif ()
    endif ()

    if (NOT DEFINED ARG_INSTALL_DESTINATION)
        set(ARG_INSTALL_DESTINATION "bin")
    endif ()

    set(_forward CXX_STANDARD ${ARG_CXX_STANDARD} INSTALL_DESTINATION ${ARG_INSTALL_DESTINATION})
    foreach (_kw NAMESPACE EXPORT_SET)
        if (DEFINED ARG_${_kw})
            list(APPEND _forward ${_kw} "${ARG_${_kw}}")
        endif ()
    endforeach ()
    foreach (_mv INCLUDE_DIRS LINK_LIBS COMPILE_OPTIONS COMPILE_DEFINITIONS PROPERTIES)
        if (ARG_${_mv})
            list(APPEND _forward ${_mv} ${ARG_${_mv}})
        endif ()
    endforeach ()

    _register_target_common(${name} ${_forward})
    _register_forward_quality_opts(${name})
endfunction()


# register_test(<name>
#     [SOURCES            <file> …]
#     [HEADERS            <file> …]
#     [CXX_MODULES        <file> …]
#     [INCLUDE_DIRS       <dir>  …]
#     [LINK_LIBS          <tgt>  …]
#     [CXX_STANDARD       <std>]
#     [COMPILE_OPTIONS     <opt> …]
#     [COMPILE_DEFINITIONS <def> …]
#     [PROPERTIES         <key val> …]
#     [TEST_ARGS          <arg> …]
#     [WORKING_DIRECTORY  <dir>]
#     [LABELS             <label> …]
#     [TIMEOUT            <seconds>]
#     [ENVIRONMENT        <VAR=val> …]
#     [ENABLE_EXCEPTIONS ON|OFF]
#     [ENABLE_IPO ON|OFF]
#     [WARNINGS_AS_ERRORS ON|OFF]
#     [ENABLE_SANITIZER_ADDRESS ON|OFF]
#     [ENABLE_SANITIZER_LEAK ON|OFF]
#     [ENABLE_SANITIZER_UNDEFINED_BEHAVIOR ON|OFF]
#     [ENABLE_SANITIZER_THREAD ON|OFF]
#     [ENABLE_SANITIZER_MEMORY ON|OFF]
#     [ENABLE_HARDENING ON|OFF]
#     [ENABLE_CLANG_TIDY ON|OFF]
#     [ENABLE_CPPCHECK ON|OFF]
# )
function(register_test name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
            ""
            "CXX_STANDARD;WORKING_DIRECTORY;TIMEOUT;
         ENABLE_EXCEPTIONS;ENABLE_IPO;WARNINGS_AS_ERRORS;
         ENABLE_SANITIZER_ADDRESS;ENABLE_SANITIZER_LEAK;
         ENABLE_SANITIZER_UNDEFINED_BEHAVIOR;ENABLE_SANITIZER_THREAD;
         ENABLE_SANITIZER_MEMORY;ENABLE_HARDENING;
         ENABLE_CLANG_TIDY;ENABLE_CPPCHECK"
            "SOURCES;HEADERS;CXX_MODULES;INCLUDE_DIRS;LINK_LIBS;COMPILE_OPTIONS;COMPILE_DEFINITIONS;PROPERTIES;TEST_ARGS;LABELS;ENVIRONMENT"
    )

    add_executable(${name})

    if (ARG_SOURCES)
        target_sources(${name} PRIVATE ${ARG_SOURCES})
    endif ()

    if (ARG_HEADERS)
        target_sources(${name} PRIVATE
                FILE_SET HEADERS
                BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
                FILES ${ARG_HEADERS}
        )
    endif ()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_include_directories(${name} PRIVATE
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        )
    endif ()

    if (ARG_CXX_MODULES)
        if (NOT DEFINED ARG_CXX_STANDARD)
            set(ARG_CXX_STANDARD 23)
        endif ()
        target_sources(${name} PRIVATE
                FILE_SET CXX_MODULES
                BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
                FILES ${ARG_CXX_MODULES}
        )
    endif ()

    if (NOT DEFINED ARG_CXX_STANDARD)
        if (DEFINED CMAKE_CXX_STANDARD)
            set(ARG_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        else ()
            set(ARG_CXX_STANDARD 17)
        endif ()
    endif ()

    set(_forward CXX_STANDARD ${ARG_CXX_STANDARD})
    foreach (_mv INCLUDE_DIRS LINK_LIBS COMPILE_OPTIONS COMPILE_DEFINITIONS PROPERTIES)
        if (ARG_${_mv})
            list(APPEND _forward ${_mv} ${ARG_${_mv}})
        endif ()
    endforeach ()

    _register_target_common(${name} ${_forward})
    _register_forward_quality_opts(${name})

    set(_wd "${CMAKE_CURRENT_BINARY_DIR}")
    if (DEFINED ARG_WORKING_DIRECTORY)
        set(_wd "${ARG_WORKING_DIRECTORY}")
    endif ()

    add_test(
            NAME ${name}
            COMMAND ${name} ${ARG_TEST_ARGS}
            WORKING_DIRECTORY "${_wd}"
    )

    if (ARG_LABELS)
        set_tests_properties(${name} PROPERTIES LABELS "${ARG_LABELS}")
    endif ()

    if (DEFINED ARG_TIMEOUT)
        set_tests_properties(${name} PROPERTIES TIMEOUT ${ARG_TIMEOUT})
    endif ()

    if (ARG_ENVIRONMENT)
        set_tests_properties(${name} PROPERTIES ENVIRONMENT "${ARG_ENVIRONMENT}")
    endif ()
endfunction()

# register_emscripten(<name>
#     [SOURCES            <file> …]
#     [HEADERS            <file> …]
#     [CXX_MODULES        <file> …]
#     [INCLUDE_DIRS       <dir>  …]
#     [LINK_LIBS          <tgt>  …]
#     [CXX_STANDARD       <std>]
#     [COMPILE_OPTIONS    <opt>  …]
#     [COMPILE_DEFINITIONS <def> …]
#     [PROPERTIES         <key val> …]
#     [DEPENDENCIES       <tgt> …]
#
#     # HTML / Web
#     [HTML_TEMPLATE      <path/to/shell.html>]
#     [HTML_TITLE         <string>]               default: "<name> - WebAssembly"
#     [CANVAS_ID          <id>]                   default: "canvas"
#     [OUTPUT_DIR         <dir>]
#
#     # Symbol exports
#     [EXPORTED_FUNCTIONS       <_func> …]
#     [EXPORTED_RUNTIME_METHODS <method> …]       e.g. ccall cwrap
#
#     # Virtual filesystem
#     [PRELOAD_FILES      <file> …]
#     [EMBED_FILES        <file> …]
#
#     # Memory  (raw bytes or units: 16MB 128MB 1GB)
#     [INITIAL_MEMORY <size>]
#     [MAXIMUM_MEMORY <size>]
#     [STACK_SIZE <size>]
#
#     # Feature flags
#     [WASM]
#     [STANDALONE_WASM]
#     [NODE_JS]
#     [PTHREAD]
#     [SIMD]
#     [ASYNCIFY]
#     [ASSERTIONS]
#     [SAFE_HEAP]
#     [ALLOW_MEMORY_GROWTH]
#     [CLOSURE_COMPILER]
#
#     # Installation — triggered by INSTALL_DESTINATION (same pattern as EXPORT_SET)
#     [INSTALL_DESTINATION <dir>]
#
#     [ENABLE_EXCEPTIONS ON|OFF]
#     [ENABLE_IPO ON|OFF]  [WARNINGS_AS_ERRORS ON|OFF]
#     [ENABLE_SANITIZER_ADDRESS ON|OFF]
#     [ENABLE_SANITIZER_LEAK ON|OFF]
#     [ENABLE_SANITIZER_UNDEFINED_BEHAVIOR ON|OFF]
#     [ENABLE_SANITIZER_THREAD ON|OFF]
#     [ENABLE_SANITIZER_MEMORY ON|OFF]
#     [ENABLE_HARDENING ON|OFF]
#     [ENABLE_CLANG_TIDY ON|OFF]
#     [ENABLE_CPPCHECK ON|OFF]
# )
#
# No-op when EMSCRIPTEN is not defined (non-web builds are unaffected).
function(register_emscripten name)
    if (NOT DEFINED EMSCRIPTEN)
        message(STATUS "[register_emscripten] Skipping '${name}' — not an Emscripten build")
        return()
    endif ()

    cmake_parse_arguments(PARSE_ARGV 1 ARG
            "WASM;STANDALONE_WASM;NODE_JS;PTHREAD;SIMD;ASYNCIFY;ASSERTIONS;
         SAFE_HEAP;ALLOW_MEMORY_GROWTH;CLOSURE_COMPILER"
            "CXX_STANDARD;HTML_TEMPLATE;HTML_TITLE;CANVAS_ID;OUTPUT_DIR;
         INITIAL_MEMORY;MAXIMUM_MEMORY;STACK_SIZE;INSTALL_DESTINATION;
         ENABLE_EXCEPTIONS;ENABLE_IPO;WARNINGS_AS_ERRORS;
         ENABLE_SANITIZER_ADDRESS;ENABLE_SANITIZER_LEAK;
         ENABLE_SANITIZER_UNDEFINED_BEHAVIOR;ENABLE_SANITIZER_THREAD;
         ENABLE_SANITIZER_MEMORY;ENABLE_HARDENING;
         ENABLE_CLANG_TIDY;ENABLE_CPPCHECK"
            "SOURCES;HEADERS;CXX_MODULES;INCLUDE_DIRS;LINK_LIBS;
         COMPILE_OPTIONS;COMPILE_DEFINITIONS;PROPERTIES;DEPENDENCIES;
         EXPORTED_FUNCTIONS;EXPORTED_RUNTIME_METHODS;
         PRELOAD_FILES;EMBED_FILES"
    )

    add_executable(${name})

    if (ARG_SOURCES)
        target_sources(${name} PRIVATE ${ARG_SOURCES})
    endif ()

    if (ARG_HEADERS)
        target_sources(${name} PRIVATE
                FILE_SET HEADERS
                BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
                FILES ${ARG_HEADERS}
        )
    endif ()

    if (ARG_CXX_MODULES)
        if (NOT DEFINED ARG_CXX_STANDARD)
            set(ARG_CXX_STANDARD 23)
        endif ()
        target_sources(${name} PRIVATE
                FILE_SET CXX_MODULES
                BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
                FILES ${ARG_CXX_MODULES}
        )
    endif ()

    if (NOT DEFINED ARG_CXX_STANDARD)
        if (DEFINED CMAKE_CXX_STANDARD)
            set(ARG_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        else ()
            set(ARG_CXX_STANDARD 17)
        endif ()
    endif ()

    if (ARG_DEPENDENCIES)
        add_dependencies(${name} ${ARG_DEPENDENCIES})
    endif ()

    set(_forward CXX_STANDARD ${ARG_CXX_STANDARD})
    foreach (_mv INCLUDE_DIRS LINK_LIBS COMPILE_OPTIONS COMPILE_DEFINITIONS PROPERTIES)
        if (ARG_${_mv})
            list(APPEND _forward ${_mv} ${ARG_${_mv}})
        endif ()
    endforeach ()

    _register_target_common(${name} ${_forward})
    _register_forward_quality_opts(${name})

    if (NOT ARG_HTML_TITLE)
        set(ARG_HTML_TITLE "${name} - WebAssembly Application")
    endif ()
    if (NOT ARG_CANVAS_ID)
        set(ARG_CANVAS_ID "canvas")
    endif ()

    if (ARG_HTML_TEMPLATE)
        set(_shell "${ARG_HTML_TEMPLATE}")
    else ()
        set(_shell "${CMAKE_CURRENT_BINARY_DIR}/${name}_shell.html")
        _register_emscripten_write_shell("${_shell}" "${ARG_HTML_TITLE}" "${ARG_CANVAS_ID}")
    endif ()

    set_target_properties(${name} PROPERTIES SUFFIX ".html")
    target_link_options(${name} PRIVATE "SHELL:--shell-file ${_shell}")

    if (ARG_OUTPUT_DIR)
        set_target_properties(${name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${ARG_OUTPUT_DIR}"
        )
    endif ()

    if (NOT DEFINED ARG_WASM OR ARG_WASM)
        target_link_options(${name} PRIVATE "SHELL:-s WASM=1")
    endif ()

    if (ARG_STANDALONE_WASM)
        target_link_options(${name} PRIVATE "SHELL:-s STANDALONE_WASM=1")
    endif ()

    if (ARG_INITIAL_MEMORY)
        _register_emscripten_parse_memory("${ARG_INITIAL_MEMORY}" _bytes)
        target_link_options(${name} PRIVATE "SHELL:-s INITIAL_MEMORY=${_bytes}")
    endif ()
    if (ARG_MAXIMUM_MEMORY)
        _register_emscripten_parse_memory("${ARG_MAXIMUM_MEMORY}" _bytes)
        target_link_options(${name} PRIVATE "SHELL:-s MAXIMUM_MEMORY=${_bytes}")
    endif ()
    if (ARG_STACK_SIZE)
        _register_emscripten_parse_memory("${ARG_STACK_SIZE}" _bytes)
        target_link_options(${name} PRIVATE "SHELL:-s STACK_SIZE=${_bytes}")
    endif ()
    if (ARG_ALLOW_MEMORY_GROWTH)
        target_link_options(${name} PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    endif ()

    if (ARG_EXPORTED_FUNCTIONS)
        string(JOIN "," _funcs ${ARG_EXPORTED_FUNCTIONS})
        target_link_options(${name} PRIVATE "SHELL:-s EXPORTED_FUNCTIONS=[${_funcs}]")
    endif ()
    if (ARG_EXPORTED_RUNTIME_METHODS)
        string(JOIN "," _methods ${ARG_EXPORTED_RUNTIME_METHODS})
        target_link_options(${name} PRIVATE "SHELL:-s EXPORTED_RUNTIME_METHODS=[${_methods}]")
    endif ()

    foreach (_f IN LISTS ARG_PRELOAD_FILES)
        target_link_options(${name} PRIVATE "SHELL:--preload-file ${_f}")
    endforeach ()
    foreach (_f IN LISTS ARG_EMBED_FILES)
        target_link_options(${name} PRIVATE "SHELL:--embed-file ${_f}")
    endforeach ()

    if (ARG_PTHREAD)
        target_compile_options(${name} PRIVATE "SHELL:-s USE_PTHREADS=1")
        target_link_options(${name} PRIVATE "SHELL:-s USE_PTHREADS=1")
    endif ()

    if (ARG_NODE_JS)
        target_link_options(${name} PRIVATE "SHELL:-s ENVIRONMENT=node")
    elseif (ARG_PTHREAD)
        target_link_options(${name} PRIVATE "SHELL:-s ENVIRONMENT=web,worker")
    else ()
        target_link_options(${name} PRIVATE "SHELL:-s ENVIRONMENT=web")
    endif ()

    if (ARG_SIMD)
        target_compile_options(${name} PRIVATE "-msimd128")
    endif ()
    if (ARG_ASYNCIFY)
        target_link_options(${name} PRIVATE "SHELL:-s ASYNCIFY=1")
    endif ()
    if (ARG_ASSERTIONS)
        target_link_options(${name} PRIVATE "SHELL:-s ASSERTIONS=1")
    endif ()
    if (ARG_SAFE_HEAP)
        target_link_options(${name} PRIVATE "SHELL:-s SAFE_HEAP=1")
    endif ()
    if (ARG_CLOSURE_COMPILER)
        target_link_options(${name} PRIVATE "SHELL:--closure 1")
    endif ()

    # Uses INSTALL_DESTINATION as the gate (same logic as EXPORT_SET elsewhere).
    # Can't go through _register_target_common because Emscripten output is a
    # file trio (.html/.js/.wasm), not an installable CMake target binary.
    if (DEFINED ARG_INSTALL_DESTINATION)
        install(FILES
                "$<TARGET_FILE_DIR:${name}>/${name}.html"
                "$<TARGET_FILE_DIR:${name}>/${name}.js"
                "$<TARGET_FILE_DIR:${name}>/${name}.wasm"
                DESTINATION "${ARG_INSTALL_DESTINATION}"
                OPTIONAL
        )
    endif ()

    message(STATUS "[register_emscripten] configured '${name}'")
endfunction()


# _register_emscripten_parse_memory(<input> <output_var>)
# Converts "16MB" / "1GB" / raw bytes → byte count.
function(_register_emscripten_parse_memory size_str out_var)
    string(TOUPPER "${size_str}" _up)
    if (_up MATCHES "^([0-9]+)(KB|MB|GB)$")
        set(_n "${CMAKE_MATCH_1}")
        set(_u "${CMAKE_MATCH_2}")
        if (_u STREQUAL "KB")
            math(EXPR _b "${_n} * 1024")
        elseif (_u STREQUAL "MB")
            math(EXPR _b "${_n} * 1024 * 1024")
        elseif (_u STREQUAL "GB")
            math(EXPR _b "${_n} * 1024 * 1024 * 1024")
        endif ()
        set(${out_var} "${_b}" PARENT_SCOPE)
    elseif (_up MATCHES "^[0-9]+$")
        set(${out_var} "${size_str}" PARENT_SCOPE)
    else ()
        message(FATAL_ERROR
                "register_emscripten: invalid memory size '${size_str}'. "
                "Use '16MB', '128MB', '1GB', or a raw byte count.")
    endif ()
endfunction()


# _register_emscripten_write_shell(<output_file> <title> <canvas_id>)
# Writes a minimal Emscripten HTML shell to disk at configure time.
function(_register_emscripten_write_shell output_file title canvas_id)
    file(WRITE "${output_file}" "\
<!DOCTYPE html>
<html lang=\"en\">
<head>
  <meta charset=\"UTF-8\">
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
  <title>${title}</title>
  <style>
    body { margin:0; background:#1a1a2e; color:#eee;
           font-family:'Segoe UI',sans-serif; display:flex;
           flex-direction:column; align-items:center; padding:20px; }
    h1   { margin-bottom:16px; }
    #${canvas_id} { border:1px solid #444; background:#000; display:block; }
    #output { width:800px; height:160px; overflow-y:auto; background:#0d0d0d;
              color:#00ff41; font-family:monospace; padding:8px;
              border:1px solid #333; margin-top:12px; box-sizing:border-box; }
    .status { margin-top:8px; font-size:.85em; color:#aaa; }
  </style>
</head>
<body>
  <h1>${title}</h1>
  <canvas id=\"${canvas_id}\" width=\"800\" height=\"600\"></canvas>
  <div id=\"output\"></div>
  <div id=\"status\" class=\"status\">Loading...</div>
  <script>
    var Module = {
      canvas: document.getElementById('${canvas_id}'),
      print: function(t) {
        var o = document.getElementById('output');
        o.textContent += t + '\\n'; o.scrollTop = o.scrollHeight;
      },
      printErr: function(t) {
        var o = document.getElementById('output');
        o.textContent += '[err] ' + t + '\\n'; o.scrollTop = o.scrollHeight;
      },
      onRuntimeInitialized: function() {
        document.getElementById('status').textContent = 'Ready';
      }
    };
  </script>
  {{{ SCRIPT }}}
</body>
</html>
")
endfunction()