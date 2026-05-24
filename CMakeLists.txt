cmake_minimum_required(VERSION 3.21)

message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#

include(project/Policies)

project(Dipp
        VERSION "1.1.0"
        DESCRIPTION "Dependency Injection in C++"
        HOMEPAGE_URL "https://github.com/JassJam/dipp"
        LANGUAGES CXX
)

if (BUILD_TESTING)
    include(CTest)
    enable_testing()
endif ()

include(PreventInSourceBuilds)
include(project/Initialization)

#

option(BENCHMARK "Enable benchmarking samples" OFF)
set(ERROR_TYPE "result" CACHE STRING "Set the error handling type")

#

set(DIPP_LIBS "")

function(install_result_packages)
    set(packages "")
    if (ERROR_TYPE STREQUAL "result")
        list(APPEND packages "leaf")
        list(APPEND DIPP_LIBS PUBLIC Boost::leaf)
    endif ()
    if (BUILD_TESTING)
        list(APPEND packages "test")
    endif ()

    list(LENGTH packages len)
    if (len EQUAL 0)
        return()
    endif ()
    list(JOIN packages "\\\\;" jpackages)
    CPMAddPackage(
            NAME Boost
            VERSION 1.86.0
            URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
            URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
            OPTIONS "BOOST_ENABLE_CMAKE ON" "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES ${jpackages}"
    )
endfunction()

install_result_packages()

#

register_header_only_library(dipp
        INCLUDE_DIRS
        include
        HEADER
        include/details/apply.hpp
        include/details/collection.hpp
        include/details/concepts.hpp
        include/details/dependency.hpp
        include/details/descriptors.hpp
        include/details/fail.hpp
        include/details/getter.hpp
        include/details/injected.hpp
        include/details/instance_info.hpp
        include/details/loader.hpp
        include/details/move_only_any.hpp
        include/details/policy.hpp
        include/details/provider.hpp
        include/details/result.hpp
        include/details/scope.hpp
        include/details/service_info.hpp
        include/details/service_lifetime.hpp
        include/details/storage.hpp
        include/details/string_hash.hpp
        include/details/type_key_pair.hpp

        include/details/descriptors/functor.hpp
        include/details/descriptors/local.hpp
        include/details/descriptors/shared.hpp
        include/details/descriptors/unique.hpp

        include/details/errors/base_error.hpp
        include/details/errors/incompatible_service_descriptor.hpp
        include/details/errors/mismatched_service_type.hpp
        include/details/errors/service_not_found.hpp

        include/details/injected/base.hpp
        include/details/injected/functor.hpp
        include/details/injected/local.hpp
        include/details/injected/ref.hpp
        include/details/injected/shared.hpp
        include/details/injected/unique.hpp
        LINK_LIBS
        ${DIPP_LIBS}
        CXX_STANDARD 23
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
