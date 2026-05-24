include_guard(DIRECTORY)

#
# usage:
#  target_load_env_variable(TARGET VARIABLE VALUE)
#  If VALUE is empty, tries to get it from system environment
#
function(target_load_env_variable TARGET_NAME VARIABLE VALUE)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_load_env_file: Target '${TARGET_NAME}' does not exist")
        return()
    endif ()

    # if VALUE is empty, try to get it from system environment
    if (VALUE STREQUAL "")
        if (DEFINED ENV{${VARIABLE}})
            set(VALUE "$ENV{${VARIABLE}}")
        else ()
            # error if variable is not defined in environment
            message(FATAL_ERROR "Environment variable '${VARIABLE}' is not defined in the system environment and no value was provided.")
        endif ()
    endif ()
    target_compile_definitions(${TARGET_NAME} PRIVATE "${VARIABLE}=${VALUE}")
endfunction()

#
# usage:
#  target_load_env_file(TARGET FILENAME)
#
function(target_load_env_file TARGET_NAME FILENAME)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_load_env_file: Target '${TARGET_NAME}' does not exist")
        return()
    endif ()

    set(ENV_KEYS_LOADED "")
    set(ENV_VALUES_LOADED "")
    load_env_file("${FILENAME}" ENV_KEYS_LOADED ENV_VALUES_LOADED)

    # set all loaded variables to the target
    list(LENGTH ENV_KEYS_LOADED ENV_LENGTH)
    if (ENV_LENGTH EQUAL 0)
        return()
    endif ()

    math(EXPR ENV_LENGTH "${ENV_LENGTH} - 1")
    foreach (j RANGE 0 ${ENV_LENGTH} 1)
        list(GET ENV_KEYS_LOADED ${j} ENV_KEY_FINAL)
        list(GET ENV_VALUES_LOADED ${j} ENV_VALUE_FINAL)
        target_compile_definitions(${TARGET_NAME} PRIVATE "${ENV_KEY_FINAL}=${ENV_VALUE_FINAL}")
    endforeach ()
endfunction()

#
# usage:
#  target_load_env_files(
#   TARGET FILENAMES...)
#
function(target_load_env_files TARGET_NAME)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_load_env_files: Target '${TARGET_NAME}' does not exist")
        return()
    endif ()

    # load all provided files and save them (override each other, to avoid duplicates)
    set(ENV_KEYS_LOADED "")
    set(ENV_VALUES_LOADED "")
    foreach (FILENAME IN LISTS ARGN)
        set(ENV_KEYS_LOADED_TMP "")
        set(ENV_VARS_LOADED_TMP "")
        load_env_file("${FILENAME}" ENV_KEYS_LOADED_TMP ENV_VARS_LOADED_TMP)

        # iterate through with index to check for duplicates
        list(LENGTH ENV_KEYS_LOADED_TMP ENV_TMP_LENGTH)
        if (ENV_TMP_LENGTH EQUAL 0)
            continue()
        endif ()
            
        math(EXPR ENV_TMP_LENGTH "${ENV_TMP_LENGTH} - 1")
        foreach (i RANGE 0 ${ENV_TMP_LENGTH} 1)
            list(GET ENV_KEYS_LOADED_TMP ${i} ENV_KEY_TMP)
            list(GET ENV_VARS_LOADED_TMP ${i} ENV_VAR_TMP)
            list(FIND ENV_KEYS_LOADED "${ENV_KEY_TMP}" ENV_KEY_INDEX)
            if (ENV_KEY_INDEX EQUAL -1)
                # not found, add new
                list(APPEND ENV_KEYS_LOADED "${ENV_KEY_TMP}")
                list(APPEND ENV_VALUES_LOADED "${ENV_VAR_TMP}")
            else ()
                # found, replace existing
                list(REMOVE_AT ENV_VALUES_LOADED ${ENV_KEY_INDEX})
                list(INSERT ENV_VALUES_LOADED ${ENV_KEY_INDEX} "${ENV_VAR_TMP}")
            endif ()
        endforeach ()
    endforeach ()

    # set all loaded variables to the target
    list(LENGTH ENV_KEYS_LOADED ENV_LENGTH)
    if (ENV_LENGTH EQUAL 0)
        return()
    endif ()
    
    math(EXPR ENV_LENGTH "${ENV_LENGTH} - 1")
    foreach (j RANGE 0 ${ENV_LENGTH} 1)
        list(GET ENV_KEYS_LOADED ${j} ENV_KEY_FINAL)
        list(GET ENV_VALUES_LOADED ${j} ENV_VALUE_FINAL)
        target_compile_definitions(${TARGET_NAME} PRIVATE "${ENV_KEY_FINAL}=${ENV_VALUE_FINAL}")
    endforeach ()

endfunction()

#
# usage:
#  target_load_env_files_with_fallback(TARGET BASE_PATH)
#  Automatically loads .env.<config> based on CMAKE_BUILD_TYPE, with fallback to .env
#
function(target_load_env_files_with_fallback TARGET_NAME BASE_PATH)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "target_load_env_files_with_fallback: Target '${TARGET_NAME}' does not exist")
        return()
    endif ()

    # Normalize the base path
    get_filename_component(BASE_DIR "${BASE_PATH}" DIRECTORY)
    get_filename_component(BASE_NAME "${BASE_PATH}" NAME)

    # Remove .env extension if present to get the base name
    string(REGEX REPLACE "\\.env$" "" BASE_NAME_CLEAN "${BASE_NAME}")

    # Construct configuration-specific file path
    string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
    set(DEFAULT_ENV_FILE "${BASE_DIR}/${BASE_NAME_CLEAN}.env")
    set(CONFIG_ENV_FILE "${BASE_DIR}/${BASE_NAME_CLEAN}.env.${BUILD_TYPE_LOWER}")

    set(ENV_FILES_TO_LOAD
            "${DEFAULT_ENV_FILE}"
            "${DEFAULT_ENV_FILE}.local"
            "${CONFIG_ENV_FILE}"
            "${CONFIG_ENV_FILE}.local"
    )

    target_load_env_files(${TARGET_NAME} ${ENV_FILES_TO_LOAD})
endfunction()

#
# usage:
#  load_env_file(FILE_PATH OUT_VARS)
# loads environment variables from a .env file into a list of strings in the format KEY=VALUE
#
function(load_env_file FILE_PATH OUT_KEYS OUT_VALUES)
    if (EXISTS "${FILE_PATH}")
        file(READ "${FILE_PATH}" ENV_CONTENTS)
        string(REPLACE "\n" ";" ENV_LINES "${ENV_CONTENTS}")

        set(ENV_KEYS_LIST "")
        set(ENV_VALUES_LIST "")
        foreach (line IN LISTS ENV_LINES)
            string(REGEX MATCH "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$" _match "${line}")
            if (_match)
                string(REGEX REPLACE "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$" "\\1" ENV_KEY "${line}")
                string(REGEX REPLACE "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$" "\\2" ENV_VAL "${line}")
                list(APPEND ENV_KEYS_LIST "${ENV_KEY}")
                list(APPEND ENV_VALUES_LIST "${ENV_VAL}")
            endif ()
        endforeach ()
        set(${OUT_KEYS} "${ENV_KEYS_LIST}" PARENT_SCOPE)
        set(${OUT_VALUES} "${ENV_VALUES_LIST}" PARENT_SCOPE)
    else ()
        set(${OUT_KEYS} "" PARENT_SCOPE)
        set(${OUT_VALUES} "" PARENT_SCOPE)
    endif ()
endfunction()
