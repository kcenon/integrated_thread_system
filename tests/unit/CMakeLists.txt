# Unit Tests for Integrated Thread System

# Add unit test executables
add_integrated_test(test_basic_operations test_basic_operations.cpp)
# Temporarily disabled - needs priority API that doesn't exist yet:
# add_integrated_test(test_priority_scheduling test_priority_scheduling.cpp)

message(STATUS "Unit tests configured: test_basic_operations")