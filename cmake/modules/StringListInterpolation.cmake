include_guard(DIRECTORY)

function(string_to_list INPUT_STRING SEPERATOR OUTPUT_VAR)
    # Replace the delimiter with semicolons (CMake's list separator)
    string(REPLACE "${SEPERATOR}" ";" LIST_ITEMS "${INPUT_STRING}")
    # Return the list to the parent scope
    set(${OUTPUT_VAR} "${LIST_ITEMS}" PARENT_SCOPE)
endfunction()

function(list_to_string LIST SEPARATOR OUTPUT_VAR)
    # Join list elements with the specified separator
    string(JOIN "${SEPARATOR}" JOINED_STRING ${LIST})
    # Return the result to the parent scope
    set(${OUTPUT_VAR} "${JOINED_STRING}" PARENT_SCOPE)
endfunction()