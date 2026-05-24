# CTest Script for CDash Submission with Authentication

# Validate required environment variables
if(NOT DEFINED ENV{CTEST_SOURCE_DIRECTORY})
    message(FATAL_ERROR "CTEST_SOURCE_DIRECTORY environment variable must be set")
endif()

if(NOT DEFINED ENV{CTEST_BINARY_DIRECTORY})
    message(FATAL_ERROR "CTEST_BINARY_DIRECTORY environment variable must be set")
endif()

if(NOT DEFINED ENV{CTEST_BUILD_NAME})
    message(FATAL_ERROR "CTEST_BUILD_NAME environment variable must be set")
endif()

if(NOT DEFINED ENV{CTEST_SITE})
    message(FATAL_ERROR "CTEST_SITE environment variable must be set")
endif()

# Set up basic test configuration
set(CTEST_SOURCE_DIRECTORY "$ENV{CTEST_SOURCE_DIRECTORY}")
set(CTEST_BINARY_DIRECTORY "$ENV{CTEST_BINARY_DIRECTORY}")
set(CTEST_BUILD_NAME "$ENV{CTEST_BUILD_NAME}")
set(CTEST_SITE "$ENV{CTEST_SITE}")

# Ensure paths are absolute and valid
get_filename_component(CTEST_SOURCE_DIRECTORY "${CTEST_SOURCE_DIRECTORY}" ABSOLUTE)
get_filename_component(CTEST_BINARY_DIRECTORY "${CTEST_BINARY_DIRECTORY}" ABSOLUTE)

# Validate that directories exist
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}")
    message(FATAL_ERROR "Source directory does not exist: ${CTEST_SOURCE_DIRECTORY}")
endif()

if(NOT EXISTS "${CTEST_BINARY_DIRECTORY}")
    message(FATAL_ERROR "Binary directory does not exist: ${CTEST_BINARY_DIRECTORY}")
endif()

# Set CDash configuration from environment variables
if(DEFINED ENV{CTEST_DROP_SITE})
    set(CTEST_DROP_SITE "$ENV{CTEST_DROP_SITE}")
endif()

if(DEFINED ENV{CTEST_DROP_LOCATION})
    set(CTEST_DROP_LOCATION "$ENV{CTEST_DROP_LOCATION}")
endif()

if(DEFINED ENV{CTEST_DROP_METHOD})
    set(CTEST_DROP_METHOD "$ENV{CTEST_DROP_METHOD}")
else()
    set(CTEST_DROP_METHOD "https")
endif()

# Use modern submit URL if provided
if(DEFINED ENV{CTEST_SUBMIT_URL})
    set(CTEST_SUBMIT_URL "$ENV{CTEST_SUBMIT_URL}")
endif()

# Start the testing process
message(STATUS "Starting CTest with:")
message(STATUS "  Source: ${CTEST_SOURCE_DIRECTORY}")
message(STATUS "  Binary: ${CTEST_BINARY_DIRECTORY}")
message(STATUS "  Build Name: ${CTEST_BUILD_NAME}")
message(STATUS "  Site: ${CTEST_SITE}")

# Determine dashboard model from environment or default to Experimental
set(DASHBOARD_MODEL "Experimental")
if(DEFINED ENV{CTEST_DASHBOARD_MODEL})
    set(DASHBOARD_MODEL "$ENV{CTEST_DASHBOARD_MODEL}")
endif()

# Initialize the test session
message(STATUS "Using dashboard model: ${DASHBOARD_MODEL}")
ctest_start("${DASHBOARD_MODEL}")

# Run the tests
message(STATUS "Running tests...")

# Determine configuration type from environment or default to Debug
set(CTEST_CONFIGURATION_TYPE "Debug")
if(DEFINED ENV{CTEST_CONFIGURATION_TYPE})
    set(CTEST_CONFIGURATION_TYPE "$ENV{CTEST_CONFIGURATION_TYPE}")
endif()

message(STATUS "Using configuration type: ${CTEST_CONFIGURATION_TYPE}")
ctest_test(BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE TEST_RESULT)

# Check test results
if(TEST_RESULT EQUAL 0)
    message(STATUS "All tests passed successfully")
else()
    message(FATAL_ERROR "One or more tests failed (return code: ${TEST_RESULT})")
endif()

# Submit results to CDash with authentication if token is available
if(DEFINED ENV{CTEST_CDASH_AUTH_TOKEN} AND NOT "$ENV{CTEST_CDASH_AUTH_TOKEN}" STREQUAL "")
    message(STATUS "Submitting to CDash with authentication...")
    ctest_submit(HTTPHEADER "Authorization: Bearer $ENV{CTEST_CDASH_AUTH_TOKEN}" RETURN_VALUE SUBMIT_RESULT)
    
    if(SUBMIT_RESULT EQUAL 0)
        message(STATUS "CDash submission completed successfully")
    else()
        message(WARNING "CDash submission failed (return code: ${SUBMIT_RESULT})")
        # Don't fail the build for submission failures, but tests already validated above
    endif()
else()
    message(STATUS "CTEST_CDASH_AUTH_TOKEN not set, skipping upload to CDash")
endif()
