include_guard(DIRECTORY)

#
# check if EMSDK is available and install it locally if needed
# usage:
# ensure_emsdk_available()
#
function(ensure_emsdk_available)
    # Check if EMSDK is already available and properly activated
    if (DEFINED ENV{EMSDK} AND EXISTS "$ENV{EMSDK}")
        # Verify that the compilers are actually working
        find_program(EMCC_TEST emcc PATHS "$ENV{EMSDK}/upstream/emscripten" NO_DEFAULT_PATH)
        if (EMCC_TEST)
            message(STATUS "Found existing EMSDK at: $ENV{EMSDK}")
            set(EMSDK_ROOT "$ENV{EMSDK}" PARENT_SCOPE)
            return()
        else ()
            message(STATUS "EMSDK found at $ENV{EMSDK} but compilers not accessible. Will install locally.")
        endif ()
    endif ()

    # Check if we have a local installation
    set(LOCAL_EMSDK_DIR "${PROJECT_SOURCE_DIR}/.emsdk")
    set(EMSDK_SCRIPT "${LOCAL_EMSDK_DIR}/emsdk")

    if (CMAKE_HOST_WIN32)
        set(EMSDK_SCRIPT "${EMSDK_SCRIPT}.bat")
        set(EMSDK_ENV_SCRIPT "${LOCAL_EMSDK_DIR}/emsdk_env.bat")
    else ()
        set(EMSDK_ENV_SCRIPT "${LOCAL_EMSDK_DIR}/emsdk_env.sh")
    endif ()

    if (EXISTS "${EMSDK_SCRIPT}")
        message(STATUS "Found local EMSDK installation at: ${LOCAL_EMSDK_DIR}")

        # Activate the local EMSDK
        _activate_local_emsdk("${LOCAL_EMSDK_DIR}")
        set(EMSDK_ROOT "${LOCAL_EMSDK_DIR}" PARENT_SCOPE)
        return()
    endif ()

    if (ENABLE_EMSDK_AUTO_INSTALL)
        message(STATUS "EMSDK not found. Automatically installing it locally to ${LOCAL_EMSDK_DIR}")
    else ()
        message(FATAL_ERROR "EMSDK not found. Please install it manually or enable ENABLE_EMSDK_AUTO_INSTALL to download it automatically.")
    endif ()

    # Create the directory
    file(MAKE_DIRECTORY "${LOCAL_EMSDK_DIR}")

    # Clone EMSDK repository
    find_package(Git QUIET)
    if (NOT GIT_FOUND)
        message(FATAL_ERROR "Git is required to download EMSDK. Please install Git first.")
    endif ()

    message(STATUS "Downloading EMSDK...")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} clone --depth 1 https://github.com/emscripten-core/emsdk.git "${LOCAL_EMSDK_DIR}"
            RESULT_VARIABLE GIT_RESULT
            OUTPUT_QUIET
            ERROR_VARIABLE GIT_ERROR
    )

    if (NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download EMSDK: ${GIT_ERROR}")
    endif ()

    # Install and activate latest EMSDK
    _install_and_activate_emsdk("${LOCAL_EMSDK_DIR}")
    set(EMSDK_ROOT "${LOCAL_EMSDK_DIR}" PARENT_SCOPE)

    message(STATUS "EMSDK installed successfully at: ${LOCAL_EMSDK_DIR}")
endfunction()

# Get the EMSDK toolchain file path
function(get_emsdk_toolchain_file output_var)
    ensure_emsdk_available()

    if (DEFINED ENV{EMSDK})
        set(TOOLCHAIN_FILE "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
    else ()
        set(TOOLCHAIN_FILE "${PROJECT_SOURCE_DIR}/.emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
    endif ()

    if (NOT EXISTS "${TOOLCHAIN_FILE}")
        message(FATAL_ERROR "Emscripten toolchain file not found: ${TOOLCHAIN_FILE}")
    endif ()

    set(${output_var} "${TOOLCHAIN_FILE}" PARENT_SCOPE)
endfunction()

#
# Check if Emscripten compilers are available and set them up
#
function(verify_and_setup_emscripten_compilers)
    ensure_emsdk_available()

    # Get the EMSDK directory
    if (DEFINED ENV{EMSDK})
        set(EMSDK_DIR "$ENV{EMSDK}")
    else ()
        set(EMSDK_DIR "${PROJECT_SOURCE_DIR}/.emsdk")
    endif ()

    # Set up the toolchain file FIRST
    get_emsdk_toolchain_file(TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${TOOLCHAIN_FILE}" CACHE FILEPATH "Emscripten toolchain file" FORCE)

    # Include the toolchain to set up Emscripten environment
    include("${TOOLCHAIN_FILE}")

    # Set up full paths to compilers
    if (CMAKE_HOST_WIN32)
        set(EMCC_PATH "${EMSDK_DIR}/upstream/emscripten/emcc.bat")
        set(EMPP_PATH "${EMSDK_DIR}/upstream/emscripten/em++.bat")
    else ()
        set(EMCC_PATH "${EMSDK_DIR}/upstream/emscripten/emcc")
        set(EMPP_PATH "${EMSDK_DIR}/upstream/emscripten/em++")
    endif ()

    # Verify compilers exist
    if (NOT EXISTS "${EMCC_PATH}")
        message(FATAL_ERROR "emcc not found at: ${EMCC_PATH}")
    endif ()

    if (NOT EXISTS "${EMPP_PATH}")
        message(FATAL_ERROR "em++ not found at: ${EMPP_PATH}")
    endif ()

    # Set CMake compiler variables only if not already properly set
    if (NOT CMAKE_C_COMPILER STREQUAL EMCC_PATH)
        set(CMAKE_C_COMPILER "${EMCC_PATH}" CACHE FILEPATH "Emscripten C compiler" FORCE)
    endif ()
    if (NOT CMAKE_CXX_COMPILER STREQUAL EMPP_PATH)
        set(CMAKE_CXX_COMPILER "${EMPP_PATH}" CACHE FILEPATH "Emscripten C++ compiler" FORCE)
    endif ()

    message(STATUS "Emscripten compilers configured:")
    message(STATUS "  - emcc: ${EMCC_PATH}")
    message(STATUS "  - em++: ${EMPP_PATH}")
    message(STATUS "  - toolchain: ${TOOLCHAIN_FILE}")
endfunction()

#
# install and activate EMSDK in the given directory
#
function(_install_and_activate_emsdk emsdk_dir)
    set(EMSDK_SCRIPT "${emsdk_dir}/emsdk")

    if (CMAKE_HOST_WIN32)
        set(EMSDK_SCRIPT "${EMSDK_SCRIPT}.bat")
    endif ()

    message(STATUS "Installing latest Emscripten...")

    # Install latest version
    execute_process(
            COMMAND "${EMSDK_SCRIPT}" install latest
            WORKING_DIRECTORY "${emsdk_dir}"
            RESULT_VARIABLE INSTALL_RESULT
            OUTPUT_VARIABLE INSTALL_OUTPUT
            ERROR_VARIABLE INSTALL_ERROR
    )

    if (NOT INSTALL_RESULT EQUAL 0)
        message(STATUS "Install output: ${INSTALL_OUTPUT}")
        message(FATAL_ERROR "Failed to install EMSDK: ${INSTALL_ERROR}")
    endif ()

    # Activate latest version
    execute_process(
            COMMAND "${EMSDK_SCRIPT}" activate latest
            WORKING_DIRECTORY "${emsdk_dir}"
            RESULT_VARIABLE ACTIVATE_RESULT
            OUTPUT_VARIABLE ACTIVATE_OUTPUT
            ERROR_VARIABLE ACTIVATE_ERROR
    )

    if (NOT ACTIVATE_RESULT EQUAL 0)
        message(STATUS "Activate output: ${ACTIVATE_OUTPUT}")
        message(FATAL_ERROR "Failed to activate EMSDK: ${ACTIVATE_ERROR}")
    endif ()

    # Set up environment for current CMake session
    _activate_local_emsdk("${emsdk_dir}")
endfunction()

#
# activate a local EMSDK installation for the current CMake session
#
function(_activate_local_emsdk emsdk_dir)
    # Set EMSDK environment variable
    set(ENV{EMSDK} "${emsdk_dir}")

    # Find the actual Node.js and Python directories (versions may vary)
    file(GLOB NODE_DIRS "${emsdk_dir}/node/*")
    file(GLOB PYTHON_DIRS "${emsdk_dir}/python/*")

    # Get the most recent versions
    if (NODE_DIRS)
        list(GET NODE_DIRS -1 NODE_BASE_DIR)
        if (CMAKE_HOST_WIN32)
            set(NODE_PATH "${NODE_BASE_DIR}/bin")
        else ()
            set(NODE_PATH "${NODE_BASE_DIR}/bin")
        endif ()
    endif ()

    if (PYTHON_DIRS)
        list(GET PYTHON_DIRS -1 PYTHON_BASE_DIR)
        if (CMAKE_HOST_WIN32)
            set(PYTHON_PATH "${PYTHON_BASE_DIR}")
        else ()
            set(PYTHON_PATH "${PYTHON_BASE_DIR}/bin")
        endif ()
    endif ()

    # Set platform-specific paths
    set(EMSCRIPTEN_ROOT "${emsdk_dir}/upstream/emscripten")

    if (CMAKE_HOST_WIN32)
        set(JAVA_DIRS "${emsdk_dir}/java/*")
        file(GLOB JAVA_DIRS ${JAVA_DIRS})
        if (JAVA_DIRS)
            list(GET JAVA_DIRS -1 JAVA_BASE_DIR)
            set(JAVA_PATH "${JAVA_BASE_DIR}/bin")
        endif ()

        # Update PATH with all necessary directories
        if (NODE_PATH AND PYTHON_PATH AND JAVA_PATH)
            set(ENV{PATH} "${EMSCRIPTEN_ROOT};${NODE_PATH};${PYTHON_PATH};${JAVA_PATH};$ENV{PATH}")
        elseif (NODE_PATH AND PYTHON_PATH)
            set(ENV{PATH} "${EMSCRIPTEN_ROOT};${NODE_PATH};${PYTHON_PATH};$ENV{PATH}")
        else ()
            set(ENV{PATH} "${EMSCRIPTEN_ROOT};$ENV{PATH}")
        endif ()
    else ()
        # Update PATH for Unix-like systems
        if (NODE_PATH AND PYTHON_PATH)
            set(ENV{PATH} "${EMSCRIPTEN_ROOT}:${NODE_PATH}:${PYTHON_PATH}:$ENV{PATH}")
        else ()
            set(ENV{PATH} "${EMSCRIPTEN_ROOT}:$ENV{PATH}")
        endif ()
    endif ()

    # Set Emscripten-specific environment variables
    set(ENV{EMSCRIPTEN} "${EMSCRIPTEN_ROOT}")
    set(ENV{EM_CONFIG} "${emsdk_dir}/.emscripten")
    set(ENV{EM_CACHE} "${emsdk_dir}/.emscripten_cache")
    set(ENV{EM_PORTS} "${emsdk_dir}/.emscripten_ports")

    message(STATUS "Local EMSDK activated:")
    message(STATUS "  - EMSDK: ${emsdk_dir}")
    message(STATUS "  - Emscripten: ${EMSCRIPTEN_ROOT}")
    if (NODE_PATH)
        message(STATUS "  - Node.js: ${NODE_PATH}")
    endif ()
    if (PYTHON_PATH)
        message(STATUS "  - Python: ${PYTHON_PATH}")
    endif ()
endfunction()
