include_guard(DIRECTORY)

#
# target_register_asset
#
# Registers an asset file to be copied to a target's output directory.
# The asset will be downloaded if missing or if the hash doesn't match.
#
# Usage:
#   target_register_asset(
#     target_name
#     FILE path/to/asset.ext # Path to the asset file (relative to current source dir or absolute)
#                            # If URL is provided, downloaded assets are stored in _assets/ subdirectory
#     [DESTINATION relative/path] # relative path within target output directory
#     [URL url_to_download_from] # URL to download the asset if missing or hash mismatch
#     [HASH hash_algorithm=hash_value] # hash to verify file integrity (e.g., SHA256=abc123...)
#     [REQUIRED] # fail build if asset cannot be found or downloaded
#   )
#
# Examples:
#   # Copy cacert.pem to target output directory
#   target_register_asset(target_name FILE cacert.pem)
#   
#   # Copy to specific subdirectory
#   target_register_asset(target_name FILE icons/app.ico DESTINATION assets/icons/app.ico)
#   
#   # Download asset if missing or hash mismatch (stores in _assets/cacert.pem)
#   target_register_asset(
#     target_name 
#     FILE cacert.pem
#     URL "https://curl.se/ca/cacert.pem"
#     HASH "SHA256=7430e90ee0cdca2d0f02b1ece46fbf255d5d0408111f009638e3b892d6ca089c"
#     REQUIRED
#   )
#
function(target_register_asset TARGET_NAME)
    set(options REQUIRED)
    set(oneValueArgs FILE DESTINATION URL HASH)
    set(multiValueArgs)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    #
    
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_register_asset() called without TARGET")
    endif ()

    if (NOT ARG_FILE)
        message(FATAL_ERROR "target_register_asset: FILE is required")
    endif ()

    # Check if target exists
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_register_asset: Target '${TARGET_NAME}' does not exist")
    endif ()

    # Resolve asset file path
    if (IS_ABSOLUTE "${ARG_FILE}")
        set(ASSET_SOURCE_PATH "${ARG_FILE}")
    else ()
        # Store downloaded assets in _assets directory
        if (ARG_URL)
            set(ASSET_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/_assets/${ARG_FILE}")
        else ()
            set(ASSET_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_FILE}")
        endif ()
    endif ()

    # Determine destination path
    if (ARG_DESTINATION)
        set(ASSET_DEST_RELATIVE "${ARG_DESTINATION}")
    else ()
        get_filename_component(ASSET_DEST_RELATIVE "${ARG_FILE}" NAME)
    endif ()

    # Generate unique target name for this asset
    string(MAKE_C_IDENTIFIER "${TARGET_NAME}_asset_${ASSET_DEST_RELATIVE}" ASSET_TARGET_NAME)

    # Check if we need to download/re-download the file
    set(NEED_DOWNLOAD FALSE)

    if (ARG_URL)
        if (NOT EXISTS "${ASSET_SOURCE_PATH}")
            message(STATUS "Asset '${ARG_FILE}' not found, will download from: ${ARG_URL}")
            set(NEED_DOWNLOAD TRUE)
        elseif (ARG_HASH)
            # Check if existing file hash matches expected hash
            string(REGEX MATCH "^([^=]+)=(.+)$" HASH_MATCH "${ARG_HASH}")
            if (HASH_MATCH)
                set(HASH_ALGORITHM "${CMAKE_MATCH_1}")
                set(EXPECTED_HASH "${CMAKE_MATCH_2}")
                file(${HASH_ALGORITHM} "${ASSET_SOURCE_PATH}" EXISTING_HASH)
                if (NOT "${EXISTING_HASH}" STREQUAL "${EXPECTED_HASH}")
                    message(STATUS "Asset '${ARG_FILE}' hash mismatch (expected: ${EXPECTED_HASH}, got: ${EXISTING_HASH}), will re-download")
                    set(NEED_DOWNLOAD TRUE)
                else ()
                    message(STATUS "Asset '${ARG_FILE}' hash verified, no download needed")
                endif ()
            else ()
                message(WARNING "Invalid hash format for asset '${ARG_FILE}': ${ARG_HASH}")
            endif ()
        endif ()

        # Download the file if needed
        if (NEED_DOWNLOAD)
            message(STATUS "Downloading asset '${ARG_FILE}' from: ${ARG_URL}")

            # Create directory if needed
            get_filename_component(ASSET_DIR "${ASSET_SOURCE_PATH}" DIRECTORY)
            file(MAKE_DIRECTORY "${ASSET_DIR}")

            # Download the file
            if (ARG_HASH)
                file(DOWNLOAD
                        "${ARG_URL}"
                        "${ASSET_SOURCE_PATH}"
                        EXPECTED_HASH "${ARG_HASH}"
                        STATUS DOWNLOAD_STATUS
                        LOG DOWNLOAD_LOG
                )
            else ()
                file(DOWNLOAD
                        "${ARG_URL}"
                        "${ASSET_SOURCE_PATH}"
                        STATUS DOWNLOAD_STATUS
                        LOG DOWNLOAD_LOG
                )
            endif ()

            # Check download status
            list(GET DOWNLOAD_STATUS 0 DOWNLOAD_ERROR)
            if (DOWNLOAD_ERROR)
                list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR_MSG)
                if (ARG_REQUIRED)
                    message(FATAL_ERROR "Failed to download asset '${ARG_FILE}': ${DOWNLOAD_ERROR_MSG}\nLog: ${DOWNLOAD_LOG}")
                else ()
                    message(WARNING "Failed to download asset '${ARG_FILE}': ${DOWNLOAD_ERROR_MSG}")
                    return()
                endif ()
            else ()
                message(STATUS "Successfully downloaded: ${ARG_FILE}")
            endif ()
        endif ()
    endif ()

    # Check if asset exists after potential download
    if (NOT EXISTS "${ASSET_SOURCE_PATH}")
        if (ARG_REQUIRED)
            message(FATAL_ERROR "Required asset '${ARG_FILE}' not found at: ${ASSET_SOURCE_PATH}")
        else ()
            message(WARNING "Asset '${ARG_FILE}' not found at: ${ASSET_SOURCE_PATH}")
            return()
        endif ()
    endif ()

    # Create custom target to copy the asset
    add_custom_target(${ASSET_TARGET_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ASSET_SOURCE_PATH}"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/${ASSET_DEST_RELATIVE}"
            DEPENDS "${ASSET_SOURCE_PATH}"
            COMMENT "Copying asset: ${ARG_FILE} -> ${ASSET_DEST_RELATIVE}"
    )

    # Make the main target depend on the asset copy
    add_dependencies(${TARGET_NAME} ${ASSET_TARGET_NAME})

    # Ensure destination directory exists
    get_filename_component(DEST_DIR "$<TARGET_FILE_DIR:${TARGET_NAME}>/${ASSET_DEST_RELATIVE}" DIRECTORY)
    add_custom_command(TARGET ${ASSET_TARGET_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
            COMMENT "Creating asset directory: ${DEST_DIR}"
    )

    message(STATUS "Registered asset for target '${TARGET_NAME}': ${ARG_FILE} -> ${ASSET_DEST_RELATIVE}")
endfunction()

#
# Function to register multiple assets at once.
#
# Usage:
#   target_register_assets(
#     target_name
#     [ASSETS asset1.txt path/to/asset2.png ...] # List of asset files
#     [DESTINATION_PREFIX prefix/path] # prefix path for all assets in target output directory
#   )
#
function(target_register_assets TARGET_NAME)
    set(options)
    set(oneValueArgs DESTINATION_PREFIX)
    set(multiValueArgs ASSETS)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    #

    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_register_assets: called without TARGET")
    endif ()

    if (NOT ARG_ASSETS)
        message(FATAL_ERROR "target_register_assets: ASSETS list is required")
    endif ()

    foreach (FILE ${ARG_ASSETS})
        if (ARG_DESTINATION_PREFIX)
            get_filename_component(ASSET_NAME "${FILE}" NAME)
            set(DESTINATION "${ARG_DESTINATION_PREFIX}/${ASSET_NAME}")
        else ()
            set(DESTINATION "")
        endif ()

        target_register_asset(
                ${TARGET_NAME}
                FILE "${FILE}"
                DESTINATION "${DESTINATION}"
        )
    endforeach ()
endfunction()
