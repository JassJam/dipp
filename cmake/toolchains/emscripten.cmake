include_guard(DIRECTORY)

# First, try to set up EMSDK if it's not available
if (NOT DEFINED ENV{EMSDK} OR NOT EXISTS "$ENV{EMSDK}")
    # Include the EMSDK manager to install it automatically
    include(${CMAKE_CURRENT_LIST_DIR}/emscripten/EmsdkManager.cmake)
    ensure_emsdk_available()
endif ()

# Ensure EMSDK is available and find the Emscripten installation
if (DEFINED ENV{EMSDK} AND EXISTS "$ENV{EMSDK}/upstream/emscripten")
    # Set up basic Emscripten configuration
    set(CMAKE_SYSTEM_NAME Emscripten)
    set(CMAKE_SYSTEM_VERSION 1)

    # Set compilers and tools
    set(EMSCRIPTEN_ROOT_PATH "$ENV{EMSDK}/upstream/emscripten")
    string(REPLACE "\\" "/" EMSCRIPTEN_ROOT_PATH "${EMSCRIPTEN_ROOT_PATH}")

    # Add Emscripten modules to CMAKE_MODULE_PATH so CMake can find Platform/Emscripten.cmake
    list(APPEND CMAKE_MODULE_PATH "${EMSCRIPTEN_ROOT_PATH}/cmake/Modules")

    if (CMAKE_HOST_WIN32)
        set(CMAKE_C_COMPILER "${EMSCRIPTEN_ROOT_PATH}/emcc.bat" CACHE FILEPATH "C compiler")
        set(CMAKE_CXX_COMPILER "${EMSCRIPTEN_ROOT_PATH}/em++.bat" CACHE FILEPATH "C++ compiler")
    else ()
        set(CMAKE_C_COMPILER "${EMSCRIPTEN_ROOT_PATH}/emcc" CACHE FILEPATH "C compiler")
        set(CMAKE_CXX_COMPILER "${EMSCRIPTEN_ROOT_PATH}/em++" CACHE FILEPATH "C++ compiler")
    endif ()

    message(STATUS "Using Emscripten from: ${EMSCRIPTEN_ROOT_PATH}")
else ()
    message(FATAL_ERROR "Emscripten SDK not found. Please install EMSDK and set the EMSDK environment variable.")
endif ()

# Set default compilation flags for WebAssembly
# Note: -s WASM=1 is a linker setting, so we don't set it in compile flags
# Enable pthread support by default for better compatibility
set(CMAKE_C_FLAGS_INIT "-pthread")
set(CMAKE_CXX_FLAGS_INIT "-pthread")

# Set default linker flags for WebAssembly
set(CMAKE_EXE_LINKER_FLAGS_INIT "-s WASM=1")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-s WASM=1")

# Set executable suffix
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Enable threading support for Emscripten
set(CMAKE_USE_PTHREADS_INIT ON)
set(CMAKE_HAVE_THREADS_LIBRARY ON)

# Disable some incompatible features for WebAssembly
set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".js")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")

# Set reasonable defaults for Emscripten build
set(CMAKE_CROSSCOMPILING_EMULATOR node)
