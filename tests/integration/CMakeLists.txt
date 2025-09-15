# Integration Tests for Integrated Thread System

# Add integration test executable using existing test file
add_executable(integration_tests
    test_system_integration.cpp
)

target_link_libraries(integration_tests
    PRIVATE
    integrated_thread_system
    GTest::gtest_main
)

target_include_directories(integration_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(integration_tests)