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

# Thread Pool v2.0 integration tests (TICKET-011)
add_executable(test_thread_pool_v2
    test_thread_pool_v2.cpp
)

target_link_libraries(test_thread_pool_v2
    PRIVATE
    integrated_thread_system
    GTest::gtest_main
)

target_include_directories(test_thread_pool_v2
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

gtest_discover_tests(test_thread_pool_v2)

# Monitoring v4.0 integration tests (TICKET-011)
add_executable(test_monitoring_v4
    test_monitoring_v4.cpp
)

target_link_libraries(test_monitoring_v4
    PRIVATE
    integrated_thread_system
    GTest::gtest_main
)

target_include_directories(test_monitoring_v4
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

gtest_discover_tests(test_monitoring_v4)