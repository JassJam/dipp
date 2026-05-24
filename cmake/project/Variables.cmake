include_guard(DIRECTORY)

# Disable CXX extensions to not use compiler-specific features
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for tools like clang-tidy or IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set target scope types for CMake targets
set(CMAKE_TARGET_SCOPE_TYPES PRIVATE PUBLIC INTERFACE)

# Debug Release RelWithDebInfo MinSizeRel
set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

# Set C++ files extensions
set(CMAKE_CXX_FILE_EXTENSION cpp cxx cc c++ m mm)

# Set C++ headers extensions
set(CMAKE_CXX_HEADER_EXTENSION h hh hpp hxx h++ ixx inc)

# Valid package managers
set(VALID_PACKAGE_MANAGERS CPM XMake)