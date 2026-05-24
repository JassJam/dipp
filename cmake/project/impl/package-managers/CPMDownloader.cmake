include_guard(DIRECTORY)

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: Copyright (c) 2019-2023 Lars Melchior and contributors

set(CPM_DOWNLOAD_VERSION "0.42.0" CACHE STRING "CPM version to download")
set(CPM_HASH_SUM "2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a" CACHE STRING "CPM download hash")
set(CPM_REPOSITORY_URL "https://github.com/cpm-cmake/CPM.cmake" CACHE STRING "CPM repository URL")

if (CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif (DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else ()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif ()

# Expand relative path. This is important if the provided path contains a tilde (~)
get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION} ABSOLUTE)

# Only download if file doesn't exist or hash doesn't match
if (NOT EXISTS ${CPM_DOWNLOAD_LOCATION} OR
        NOT CMPM_HASH_SUM MATCHES "^([0-9a-f]{64})$" OR
        NOT CMPM_HASH_SUM STREQUAL "SHA256=${CPM_HASH_SUM}")
    file(DOWNLOAD
            ${CPM_REPOSITORY_URL}/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
            ${CPM_DOWNLOAD_LOCATION} EXPECTED_HASH SHA256=${CPM_HASH_SUM}
    )
endif ()

include(${CPM_DOWNLOAD_LOCATION})
