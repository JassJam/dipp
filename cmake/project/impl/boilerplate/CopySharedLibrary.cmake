include_guard(DIRECTORY)
include(CMakePackageConfigHelpers)
    include(GetCurrentCompiler)

# Helper function to copy shared library dependencies to build directory for direct execution
function(_copy_shared_library_dependencies_to_build_dir TARGET_NAME)
    # Get the target's link libraries
    get_target_property(TARGET_LINK_LIBS ${TARGET_NAME} LINK_LIBRARIES)
    if (NOT TARGET_LINK_LIBS)
        return()
    endif ()

    # Process each linked library
    foreach (LIB ${TARGET_LINK_LIBS})
        if (TARGET ${LIB})
            get_target_property(LIB_TYPE ${LIB} TYPE)

            # Ensure build order dependency for all target types
            add_dependencies(${TARGET_NAME} ${LIB})

            # Copy shared libraries to target directory for direct execution
            if (LIB_TYPE STREQUAL "SHARED_LIBRARY")
                # Add a post-build step to copy the shared library to the target's directory
                add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:${LIB}>"
                        "$<TARGET_FILE_DIR:${TARGET_NAME}>/"
                        COMMENT "Copying shared library ${LIB} for ${TARGET_NAME}"
                        VERBATIM
                )

                message(STATUS "** Will copy shared library ${LIB} to build directory for ${TARGET_NAME}")
            endif ()

            # Recursively handle dependencies of this library
            _copy_shared_library_dependencies_to_build_dir_recursive(${TARGET_NAME} ${LIB})
        endif ()
    endforeach ()
endfunction()

# Helper function to recursively handle dependencies
function(_copy_shared_library_dependencies_to_build_dir_recursive MAIN_TARGET LIB_TARGET)
    get_target_property(LIB_LINK_LIBS ${LIB_TARGET} LINK_LIBRARIES)
    if (NOT LIB_LINK_LIBS)
        return()
    endif ()

    foreach (NESTED_LIB ${LIB_LINK_LIBS})
        if (TARGET ${NESTED_LIB})
            get_target_property(NESTED_LIB_TYPE ${NESTED_LIB} TYPE)

            # Ensure build order dependency
            add_dependencies(${MAIN_TARGET} ${NESTED_LIB})

            # Copy shared libraries
            if (NESTED_LIB_TYPE STREQUAL "SHARED_LIBRARY")
                add_custom_command(TARGET ${MAIN_TARGET} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:${NESTED_LIB}>"
                        "$<TARGET_FILE_DIR:${MAIN_TARGET}>/"
                        COMMENT "Copying transitive shared library ${NESTED_LIB} for ${MAIN_TARGET}"
                        VERBATIM
                )

                message(STATUS "** Will copy transitive shared library ${NESTED_LIB} to build directory for ${MAIN_TARGET}")
            endif ()

            # Continue recursively (with depth limit to avoid infinite loops)
            get_target_property(PROCESSED ${MAIN_TARGET} _PROCESSED_DEPS)
            if (NOT PROCESSED)
                set_target_properties(${MAIN_TARGET} PROPERTIES _PROCESSED_DEPS "")
                set(PROCESSED "")
            endif ()

            if (NOT "${NESTED_LIB}" IN_LIST PROCESSED)
                list(APPEND PROCESSED ${NESTED_LIB})
                set_target_properties(${MAIN_TARGET} PROPERTIES _PROCESSED_DEPS "${PROCESSED}")
                _copy_shared_library_dependencies_to_build_dir_recursive(${MAIN_TARGET} ${NESTED_LIB})
            endif ()
        endif ()
    endforeach ()
endfunction()

# Helper function to copy AddressSanitizer runtime DLL to build directory for direct execution
function(_copy_asan_dll_to_build_dir TARGET_NAME)
    # Only handle this for MSVC with AddressSanitizer enabled
    get_current_compiler(CURRENT_COMPILER)

    if (NOT "${CURRENT_COMPILER}" STREQUAL "MSVC")
        return()
    endif ()

    # Check if AddressSanitizer is enabled by looking for /fsanitize in flags
    string(FIND "${CMAKE_CXX_FLAGS}" "/fsanitize" ASAN_FLAGS_INDEX)
    if (ASAN_FLAGS_INDEX EQUAL -1)
        return()
    endif ()

    # Find the AddressSanitizer DLL using the same logic as _install_asan_runtime_dll
    _find_asan_dll_path(ASAN_DLL_PATH)

    if (ASAN_DLL_PATH AND EXISTS "${ASAN_DLL_PATH}")
        # Get the target's output directory
        get_target_property(TARGET_OUTPUT_DIR ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
        if (NOT TARGET_OUTPUT_DIR)
            set(TARGET_OUTPUT_DIR $<TARGET_FILE_DIR:${TARGET_NAME}>)
        endif ()

        # Normalize path for CMake (use forward slashes)
        file(TO_CMAKE_PATH "${ASAN_DLL_PATH}" ASAN_DLL_CMAKE_PATH)
        get_filename_component(ASAN_DLL_NAME "${ASAN_DLL_CMAKE_PATH}" NAME)

        # Add a post-build step to copy the DLL to the target's output directory
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ASAN_DLL_CMAKE_PATH}"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/${ASAN_DLL_NAME}"
                COMMENT "Copying AddressSanitizer runtime DLL for ${TARGET_NAME}"
                VERBATIM
        )

        message(STATUS "** Will copy AddressSanitizer runtime DLL to build directory for ${TARGET_NAME}")
    else ()
        message(WARNING "AddressSanitizer runtime DLL not found for build directory copying. ${TARGET_NAME} may not run directly from build directory.")
    endif ()
endfunction()

# Helper function to find AddressSanitizer DLL path (shared between install and build directory copying)
function(_find_asan_dll_path OUTPUT_VAR)
    # Determine architecture-specific DLL name
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ASAN_DLL_PATTERN "clang_rt.asan_dynamic-x86_64.dll")
        set(ARCH_DIR "x64")
    else ()
        set(ASAN_DLL_PATTERN "clang_rt.asan_dynamic-i386.dll")
        set(ARCH_DIR "x86")
    endif ()

    set(ASAN_DLL_PATH "")

    # First, try to use vswhere to find Visual Studio installations
    find_program(VSWHERE_EXECUTABLE
            NAMES vswhere.exe
            PATHS
            "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer"
            "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer"
            DOC "Visual Studio locator tool"
    )

    if (VSWHERE_EXECUTABLE)
        # Get Visual Studio installation path using vswhere
        execute_process(
                COMMAND "${VSWHERE_EXECUTABLE}" -latest -property installationPath
                OUTPUT_VARIABLE VS_INSTALL_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
        )

        if (VS_INSTALL_PATH AND EXISTS "${VS_INSTALL_PATH}")
            # Search for AddressSanitizer runtime DLL in VC tools
            file(GLOB_RECURSE ASAN_DLL_CANDIDATES
                    "${VS_INSTALL_PATH}/VC/Tools/MSVC/*/bin/Host*/${ARCH_DIR}/${ASAN_DLL_PATTERN}")

            if (ASAN_DLL_CANDIDATES)
                # Prefer the newest version (last in sorted list)
                list(SORT ASAN_DLL_CANDIDATES)
                list(GET ASAN_DLL_CANDIDATES -1 ASAN_DLL_PATH)
            endif ()
        endif ()
    endif ()

    # Fallback: Search in environment variables
    if (NOT ASAN_DLL_PATH OR NOT EXISTS "${ASAN_DLL_PATH}")
        # Try VCINSTALLDIR environment variable
        if (DEFINED ENV{VCINSTALLDIR})
            file(GLOB_RECURSE ASAN_DLL_CANDIDATES
                    "$ENV{VCINSTALLDIR}/Tools/MSVC/*/bin/Host*/${ARCH_DIR}/${ASAN_DLL_PATTERN}")
            if (ASAN_DLL_CANDIDATES)
                list(SORT ASAN_DLL_CANDIDATES)
                list(GET ASAN_DLL_CANDIDATES -1 ASAN_DLL_PATH)
            endif ()
        endif ()

        # Try VCToolsInstallDir environment variable
        if ((NOT ASAN_DLL_PATH OR NOT EXISTS "${ASAN_DLL_PATH}") AND DEFINED ENV{VCToolsInstallDir})
            file(GLOB_RECURSE ASAN_DLL_CANDIDATES
                    "$ENV{VCToolsInstallDir}/bin/Host*/${ARCH_DIR}/${ASAN_DLL_PATTERN}")
            if (ASAN_DLL_CANDIDATES)
                list(SORT ASAN_DLL_CANDIDATES)
                list(GET ASAN_DLL_CANDIDATES -1 ASAN_DLL_PATH)
            endif ()
        endif ()
    endif ()

    # Final fallback: Search in common Visual Studio installation directories
    if (NOT ASAN_DLL_PATH OR NOT EXISTS "${ASAN_DLL_PATH}")
        set(COMMON_VS_ROOTS
                "$ENV{ProgramFiles}/Microsoft Visual Studio"
                "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio"
        )

        foreach (VS_ROOT ${COMMON_VS_ROOTS})
            if (EXISTS "${VS_ROOT}")
                file(GLOB VS_VERSIONS "${VS_ROOT}/20*/*/VC/Tools/MSVC")
                foreach (VS_VERSION_PATH ${VS_VERSIONS})
                    if (EXISTS "${VS_VERSION_PATH}")
                        file(GLOB_RECURSE ASAN_DLL_CANDIDATES
                                "${VS_VERSION_PATH}/*/bin/Host*/${ARCH_DIR}/${ASAN_DLL_PATTERN}")
                        if (ASAN_DLL_CANDIDATES)
                            list(SORT ASAN_DLL_CANDIDATES)
                            list(GET ASAN_DLL_CANDIDATES -1 ASAN_DLL_PATH)
                            break()
                        endif ()
                    endif ()
                endforeach ()
                if (ASAN_DLL_PATH AND EXISTS "${ASAN_DLL_PATH}")
                    break()
                endif ()
            endif ()
        endforeach ()
    endif ()

    set(${OUTPUT_VAR} "${ASAN_DLL_PATH}" PARENT_SCOPE)
endfunction()

# Helper function to install AddressSanitizer runtime DLL alongside the executable
function(_install_asan_runtime_dll TARGET_NAME RUNTIME_DIR)
    # Only handle this for MSVC with AddressSanitizer enabled
    get_current_compiler(CURRENT_COMPILER)

    if (NOT "${CURRENT_COMPILER}" STREQUAL "MSVC")
        return()
    endif ()

    # Check if AddressSanitizer is enabled by looking for /fsanitize in flags
    string(FIND "${CMAKE_CXX_FLAGS}" "/fsanitize" ASAN_FLAGS_INDEX)
    if (ASAN_FLAGS_INDEX EQUAL -1)
        return()
    endif ()

    # Use the shared helper function to find the DLL
    _find_asan_dll_path(ASAN_DLL_PATH)

    if (ASAN_DLL_PATH AND EXISTS "${ASAN_DLL_PATH}")
        message(STATUS "** Found AddressSanitizer runtime DLL for installation: ${ASAN_DLL_PATH}")

        # Normalize path for CMake (use forward slashes)
        file(TO_CMAKE_PATH "${ASAN_DLL_PATH}" ASAN_DLL_CMAKE_PATH)

        # Install the DLL alongside the executable
        install(FILES "${ASAN_DLL_CMAKE_PATH}"
                DESTINATION ${RUNTIME_DIR}
                COMPONENT Runtime)
        message(STATUS "** Will install AddressSanitizer runtime DLL for ${TARGET_NAME}")
    else ()
        # Determine architecture for error message
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(ASAN_DLL_PATTERN "clang_rt.asan_dynamic-x86_64.dll")
            set(ARCH_DIR "x64")
        else ()
            set(ASAN_DLL_PATTERN "clang_rt.asan_dynamic-i386.dll")
            set(ARCH_DIR "x86")
        endif ()

        find_program(VSWHERE_EXECUTABLE
                NAMES vswhere.exe
                PATHS
                "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer"
                "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer"
                DOC "Visual Studio locator tool"
        )

        message(WARNING "AddressSanitizer runtime DLL (${ASAN_DLL_PATTERN}) not found for installation. Installed executable may not run without setting PATH.")
        message(STATUS "** Searched architecture: ${ARCH_DIR}")
        if (VSWHERE_EXECUTABLE)
            message(STATUS "** Used vswhere: ${VSWHERE_EXECUTABLE}")
        else ()
            message(STATUS "** vswhere not found, used fallback search")
        endif ()
    endif ()
endfunction()

# Helper function to install shared library dependencies cross-platform
function(_target_install_shared_library_dependencies TARGET_NAME RUNTIME_DIR)
    # Get TARGET_NAME type
    get_target_property(target_type ${TARGET_NAME} TYPE)
    if (NOT target_type STREQUAL "EXECUTABLE" AND NOT target_type STREQUAL "SHARED_LIBRARY")
        return()  # Only handle executables and shared libraries
    endif ()

    # Get target output name
    get_target_property(target_output_name ${TARGET_NAME} OUTPUT_NAME)
    if (NOT target_output_name)
        set(target_output_name ${TARGET_NAME})
    endif ()

    # Create a post-install script to copy shared library dependencies
    set(install_script_file "${CMAKE_CURRENT_BINARY_DIR}/install_${TARGET_NAME}_dependencies.cmake")

    file(WRITE ${install_script_file} "
# Auto-generated script to install shared library dependencies for ${TARGET_NAME}
cmake_minimum_required(VERSION 3.15)

# Get the TARGET_NAME executable/library path - ensure it's absolute
get_filename_component(INSTALL_PREFIX_ABS \"\${CMAKE_INSTALL_PREFIX}\" ABSOLUTE)

# Platform-specific file extensions and library search patterns
if(WIN32)
    set(SHARED_LIB_EXTENSIONS \".dll\")
    set(EXECUTABLE_EXTENSION \".exe\")
elseif(APPLE)
    set(SHARED_LIB_EXTENSIONS \".dylib\" \".so\")
    set(EXECUTABLE_EXTENSION \"\")
else()
    set(SHARED_LIB_EXTENSIONS \".so\")
    set(EXECUTABLE_EXTENSION \"\")
endif()

# Determine TARGET_NAME file based on type
if(\"${target_type}\" STREQUAL \"EXECUTABLE\")
    set(TARGET_FILE \"\${INSTALL_PREFIX_ABS}/${RUNTIME_DIR}/${target_output_name}\${EXECUTABLE_EXTENSION}\")
else()
    # For shared libraries, try different extensions
    foreach(ext \${SHARED_LIB_EXTENSIONS})
        set(potential_file \"\${INSTALL_PREFIX_ABS}/${RUNTIME_DIR}/${target_output_name}\${ext}\")
        if(EXISTS \"\${potential_file}\")
            set(TARGET_FILE \"\${potential_file}\")
            break()
        endif()
    endforeach()
endif()

if(EXISTS \"\${TARGET_FILE}\")
    message(STATUS \"Installing shared library dependencies for: \${TARGET_FILE}\")
    
    # Find all potential build directories where shared libraries might be located
    set(BUILD_DIR \"${CMAKE_BINARY_DIR}\")
    get_filename_component(TARGET_BUILD_BASE \"\${BUILD_DIR}\" ABSOLUTE)
    set(SEARCH_DIRECTORIES \"\")
    
    # Add common build output directory patterns
    foreach(config \"Release\" \"Debug\" \"RelWithDebInfo\" \"MinSizeRel\" \"\")
        foreach(subpath \"${CMAKE_CURRENT_BINARY_DIR}\" \".\")
            if(NOT \"\${config}\" STREQUAL \"\")
                set(potential_dir \"\${TARGET_BUILD_BASE}/\${subpath}/\${config}\")
            else()
                set(potential_dir \"\${TARGET_BUILD_BASE}/\${subpath}\")
            endif()
            if(EXISTS \"\${potential_dir}\")
                list(APPEND SEARCH_DIRECTORIES \"\${potential_dir}\")
            endif()
        endforeach()
    endforeach()
    
    # Search in CMake targets' output directories for transitive dependencies
    get_cmake_property(_target_names CACHE_VARIABLES)
    foreach(_cache_var \${_target_names})
        if(_cache_var MATCHES \".*_BINARY_DIR\$\")
            set(pkg_binary_dir \${${_cache_var}})
            if(EXISTS \"\${pkg_binary_dir}\")
                foreach(subdir \"Release\" \"Debug\" \"RelWithDebInfo\" \"MinSizeRel\" \"\" \"bin\" \"lib\" \"library\" \"dll\")
                    set(search_dir \"\${pkg_binary_dir}\")
                    if(NOT \"\${subdir}\" STREQUAL \"\")
                        set(search_dir \"\${pkg_binary_dir}/\${subdir}\")
                    endif()
                    if(EXISTS \"\${search_dir}\")
                        list(APPEND SEARCH_DIRECTORIES \"\${search_dir}\")
                    endif()
                endforeach()
            endif()
        endif()
    endforeach()
    
    # Also search in CPM package directories and their subdirectories
    get_cmake_property(_variableNames VARIABLES)
    foreach(_varName \${_variableNames})
        if(_varName MATCHES \".*_SOURCE_DIR\$\" OR _varName MATCHES \".*_BINARY_DIR\$\")
            set(pkg_dir \${${_varName}})
            if(EXISTS \"\${pkg_dir}\")
                # Search in common library subdirectories
                foreach(libsubdir \"\" \"bin\" \"lib\" \"library\" \"libs\")
                    foreach(configsubdir \"\" \"Release\" \"Debug\" \"RelWithDebInfo\" \"MinSizeRel\")
                        set(search_path \"\${pkg_dir}\")
                        if(NOT \"\${libsubdir}\" STREQUAL \"\")
                            set(search_path \"\${search_path}/\${libsubdir}\")
                        endif()
                        if(NOT \"\${configsubdir}\" STREQUAL \"\")
                            set(search_path \"\${search_path}/\${configsubdir}\")
                        endif()
                        if(EXISTS \"\${search_path}\")
                            list(APPEND SEARCH_DIRECTORIES \"\${search_path}\")
                        endif()
                    endforeach()
                endforeach()
            endif()
        endif()
    endforeach()
    
    # Remove duplicates and non-existent directories
    if(SEARCH_DIRECTORIES)
        list(REMOVE_DUPLICATES SEARCH_DIRECTORIES)
    endif()
    
    # Search for shared libraries in all directories
    set(COPIED_LIBRARIES \"\")
    foreach(search_dir \${SEARCH_DIRECTORIES})
        if(EXISTS \"\${search_dir}\")
            foreach(ext \${SHARED_LIB_EXTENSIONS})
                file(GLOB shared_libs \"\${search_dir}/*\${ext}\")
                foreach(lib_file \${shared_libs})
                    get_filename_component(lib_name \"\${lib_file}\" NAME)
                    set(dest_file \"\${INSTALL_PREFIX_ABS}/${RUNTIME_DIR}/\${lib_name}\")
                    
                    # Skip if it's the TARGET_NAME itself
                    get_filename_component(target_basename \"\${TARGET_FILE}\" NAME)
                    if(NOT \"\${lib_name}\" STREQUAL \"\${target_basename}\" AND NOT \"\${lib_name}\" IN_LIST COPIED_LIBRARIES)
                        # Skip system libraries on Unix-like systems
                        set(skip_lib FALSE)
                        if(UNIX)
                            # Skip common system libraries
                            string(REGEX MATCH \"^lib(c|m|dl|pthread|rt|util|gcc_s|stdc\\\\+\\\\+)\\\\.so\" is_system_lib \"\${lib_name}\")
                            if(is_system_lib)
                                set(skip_lib TRUE)
                            endif()
                            # Skip libraries in system directories
                            string(FIND \"\${lib_file}\" \"/usr/lib\" usr_lib_pos)
                            string(FIND \"\${lib_file}\" \"/lib\" lib_pos)
                            if(usr_lib_pos GREATER_EQUAL 0 OR lib_pos EQUAL 0)
                                set(skip_lib TRUE)
                            endif()
                        elseif(WIN32)
                            # Skip Windows system DLLs
                            string(TOLOWER \"\${lib_file}\" lib_file_lower)
                            if(lib_file_lower MATCHES \"(system32|syswow64|winsxs|windows)\")
                                set(skip_lib TRUE)
                            endif()
                        endif()
                        
                        if(NOT skip_lib AND NOT EXISTS \"\${dest_file}\")
                            message(STATUS \"  Installing shared library dependency: \${lib_name}\")
                            execute_process(
                                COMMAND \"\${CMAKE_COMMAND}\" -E copy_if_different
                                \"\${lib_file}\"
                                \"\${dest_file}\"
                                RESULT_VARIABLE COPY_RESULT
                            )
                            if(NOT COPY_RESULT EQUAL 0)
                                message(WARNING \"Failed to copy \${lib_file} to \${dest_file}\")
                            else()
                                list(APPEND COPIED_LIBRARIES \"\${lib_name}\")
                            endif()
                        elseif(EXISTS \"\${dest_file}\")
                            message(STATUS \"  Shared library dependency already exists: \${lib_name}\")
                            list(APPEND COPIED_LIBRARIES \"\${lib_name}\")
                        elseif(skip_lib)
                            message(STATUS \"  Skipping system library: \${lib_name}\")
                        endif()
                    endif()
                endforeach()
            endforeach()
        endif()
    endforeach()
else()
    message(WARNING \"Target file does not exist: \${TARGET_FILE}\")
endif()
")

    # Install the script to run after the main installation
    install(SCRIPT ${install_script_file} COMPONENT Runtime)

    # Enhanced handling of TARGET_NAME dependencies to include transitive dependencies
    get_target_property(target_link_libs ${TARGET_NAME} LINK_LIBRARIES)
    if (target_link_libs)
        # Function to collect all dependency targets recursively (including static libraries with shared deps)
        function(collect_all_dependency_targets target_name visited_targets dependency_targets)
            # Avoid infinite recursion
            if (target_name IN_LIST visited_targets)
                return()
            endif ()
            list(APPEND visited_targets ${target_name})

            if (TARGET ${target_name})
                get_target_property(target_type ${target_name} TYPE)

                list(APPEND dependency_targets ${target_name})

                # Recursively check this TARGET_NAME's dependencies
                get_target_property(target_deps ${target_name} LINK_LIBRARIES)
                if (target_deps)
                    foreach (dep ${target_deps})
                        collect_all_dependency_targets(${dep} \"${visited_targets}\" dependency_targets)
                    endforeach ()
                endif ()

                # Also check interface link libraries for transitive dependencies
                get_target_property(interface_deps ${target_name} INTERFACE_LINK_LIBRARIES)
                if (interface_deps)
                    foreach (dep ${interface_deps})
                        if (TARGET ${dep})
                            collect_all_dependency_targets(${dep} \"${visited_targets}\" dependency_targets)
                        endif ()
                    endforeach ()
                endif ()
            endif ()

            # Propagate results back to parent scope
            set(dependency_targets ${dependency_targets} PARENT_SCOPE)
            set(visited_targets ${visited_targets} PARENT_SCOPE)
        endfunction()

        # Collect all dependency targets
        set(all_dependency_targets \"\")
        set(visited_list \"\")
        foreach (lib ${target_link_libs})
            collect_all_dependency_targets(${lib} \"${visited_list}\" all_dependency_targets)
        endforeach ()

        # Remove duplicates
        if (all_dependency_targets)
            list(REMOVE_DUPLICATES all_dependency_targets)
        endif ()

        # Install shared libraries for all dependency targets
        foreach (dep_target ${all_dependency_targets})
            if (TARGET ${dep_target})
                get_target_property(dep_type ${dep_target} TYPE)
                if (dep_type STREQUAL "SHARED_LIBRARY")
                    install(FILES $<TARGET_FILE:${dep_target}>
                            DESTINATION ${RUNTIME_DIR}
                            COMPONENT Runtime
                    )
                endif ()
            endif ()
        endforeach ()
    endif ()
endfunction()
