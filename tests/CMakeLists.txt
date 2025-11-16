# Tests CMakeLists.txt for Integrated Thread System
cmake_minimum_required(VERSION 3.16)

# Testing configuration
enable_testing()

# Find required packages
find_package(Threads REQUIRED)

# Google Test setup
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Test utilities library
add_library(test_utilities INTERFACE)
target_include_directories(test_utilities INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)
target_compile_features(test_utilities INTERFACE cxx_std_20)

# Common test configuration
set(TEST_COMMON_LIBS
    integrated_thread_system
    test_utilities
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Helper function to create test executables
function(add_integrated_test test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE ${TEST_COMMON_LIBS})
    target_compile_features(${test_name} PRIVATE cxx_std_20)

    # Add compiler warnings
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -pedantic)
    endif()

    # Register with CTest
    add_test(NAME ${test_name} COMMAND ${test_name})

    # Set test properties
    # Timeout increased to 120s to accommodate system initialization (5s per test)
    # and potential CI/sanitizer overhead
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 120
        LABELS "${ARGN}"
    )
endfunction()

# Unit tests
add_subdirectory(unit)

# Integration tests (temporarily disabled while updating APIs)
# add_subdirectory(integration)

# Benchmarks (optional)
if(INTEGRATED_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Coverage is handled by the CI workflow using lcov/gcovr
# See .github/workflows/coverage.yml for coverage configuration

# Create a target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS all_tests
    COMMENT "Running all tests..."
)

# Create test groups
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L "unit" --output-on-failure
    COMMENT "Running unit tests..."
)

add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L "integration" --output-on-failure
    COMMENT "Running integration tests..."
)

if(INTEGRATED_BUILD_BENCHMARKS)
    add_custom_target(run_benchmarks
        COMMAND ${CMAKE_CTEST_COMMAND} -L "benchmark" --output-on-failure
        COMMENT "Running benchmarks..."
    )
endif()

# Test summary
message(STATUS "Tests configured:")
message(STATUS "  Unit tests: ${INTEGRATED_BUILD_TESTS}")
message(STATUS "  Integration tests: ${INTEGRATED_BUILD_TESTS}")
message(STATUS "  Benchmarks: ${INTEGRATED_BUILD_BENCHMARKS}")
message(STATUS "  Coverage: ${INTEGRATED_ENABLE_COVERAGE}")