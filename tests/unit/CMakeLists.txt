# Unit Tests for Integrated Thread System

# Add unit test executables
add_integrated_test(test_basic_operations test_basic_operations.cpp)
add_integrated_test(test_basic_operations_improved test_basic_operations_improved.cpp unit)

# Temporarily disabled - needs priority API that doesn't exist yet:
# add_integrated_test(test_priority_scheduling test_priority_scheduling.cpp)

message(STATUS "Unit tests configured:")
message(STATUS "  - test_basic_operations (original)")
message(STATUS "  - test_basic_operations_improved (with Phase 1-3 improvements)")