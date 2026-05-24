include_guard(DIRECTORY)

if ("CPM" IN_LIST PACKAGE_MANAGERS)
    message(STATUS "Enabling CPM package manager")
    include(${CMAKE_CURRENT_LIST_DIR}/package-managers/CPMDownloader.cmake)
endif ()

if ("XMake" IN_LIST PACKAGE_MANAGERS)
    message(STATUS "Enabling XMake package manager")
    include(${CMAKE_CURRENT_LIST_DIR}/package-managers/XMake.cmake)
endif ()
