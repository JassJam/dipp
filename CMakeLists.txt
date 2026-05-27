cmake_minimum_required(VERSION 3.21)

message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#

include(project/Policies)

project(dipp
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
        HEADER_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include

        INCLUDE_DIRS
        include
        HEADERS
        include/dipp/dipp.hpp
        include/dipp/details/apply.hpp
        include/dipp/details/collection.hpp
        include/dipp/details/concepts.hpp
        include/dipp/details/dependency.hpp
        include/dipp/details/descriptors.hpp
        include/dipp/details/fail.hpp
        include/dipp/details/getter.hpp
        include/dipp/details/injected.hpp
        include/dipp/details/instance_info.hpp
        include/dipp/details/loader.hpp
        include/dipp/details/move_only_any.hpp
        include/dipp/details/policy.hpp
        include/dipp/details/provider.hpp
        include/dipp/details/result.hpp
        include/dipp/details/scope.hpp
        include/dipp/details/service_info.hpp
        include/dipp/details/service_lifetime.hpp
        include/dipp/details/storage.hpp
        include/dipp/details/string_hash.hpp
        include/dipp/details/type_key_pair.hpp

        include/dipp/details/descriptors/base.hpp
        include/dipp/details/descriptors/functor.hpp
        include/dipp/details/descriptors/local.hpp
        include/dipp/details/descriptors/shared.hpp
        include/dipp/details/descriptors/unique.hpp

        include/dipp/details/errors/base_error.hpp
        include/dipp/details/errors/incompatible_service_descriptor.hpp
        include/dipp/details/errors/mismatched_service_type.hpp
        include/dipp/details/errors/service_not_found.hpp

        include/dipp/details/injected/base.hpp
        include/dipp/details/injected/functor.hpp
        include/dipp/details/injected/local.hpp
        include/dipp/details/injected/ref.hpp
        include/dipp/details/injected/shared.hpp
        include/dipp/details/injected/unique.hpp
        LINK_LIBS
        ${DIPP_LIBS}

        CXX_STANDARD 23

        NAMESPACE dipp
        EXPORT_SET dipp
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

register_package_config(
    NAME         dipp
    NAMESPACE    dipp
    EXPORT_SET   dipp
    COMPATIBILITY SameMajorVersion
)
