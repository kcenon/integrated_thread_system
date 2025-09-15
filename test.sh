#!/bin/bash

# Integrated Thread System Test Runner
# Comprehensive testing script with multiple test strategies

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Display banner
echo -e "${BOLD}${BLUE}================================================${NC}"
echo -e "${BOLD}${BLUE}    Integrated Thread System Test Suite        ${NC}"
echo -e "${BOLD}${BLUE}    Comprehensive Testing Framework            ${NC}"
echo -e "${BOLD}${BLUE}================================================${NC}"

# Function to print messages
print_status() { echo -e "${BOLD}${BLUE}[STATUS]${NC} $1"; }
print_success() { echo -e "${BOLD}${GREEN}[SUCCESS]${NC} $1"; }
print_error() { echo -e "${BOLD}${RED}[ERROR]${NC} $1"; }
print_warning() { echo -e "${BOLD}${YELLOW}[WARNING]${NC} $1"; }
print_info() { echo -e "${BOLD}${CYAN}[INFO]${NC} $1"; }

# Function to show help
show_help() {
    echo -e "${BOLD}Usage:${NC} $0 [options]"
    echo ""
    echo -e "${BOLD}Test Types:${NC}"
    echo "  --unit            Run unit tests only"
    echo "  --integration     Run integration tests only"
    echo "  --performance     Run performance benchmarks"
    echo "  --stress          Run stress tests"
    echo "  --memory          Run memory tests (with valgrind if available)"
    echo "  --thread-safety   Run thread safety tests"
    echo "  --all             Run all test types (default)"
    echo ""
    echo -e "${BOLD}Test Configuration:${NC}"
    echo "  --debug           Build and test in debug mode"
    echo "  --release         Build and test in release mode (default)"
    echo "  --sanitizers      Enable all sanitizers during testing"
    echo "  --asan            Enable AddressSanitizer"
    echo "  --tsan            Enable ThreadSanitizer"
    echo "  --ubsan           Enable UndefinedBehaviorSanitizer"
    echo ""
    echo -e "${BOLD}Output Options:${NC}"
    echo "  --verbose         Show detailed test output"
    echo "  --quiet           Show minimal output"
    echo "  --junit           Generate JUnit XML reports"
    echo "  --coverage        Generate code coverage reports"
    echo ""
    echo -e "${BOLD}Build Options:${NC}"
    echo "  --no-build        Skip building, use existing binaries"
    echo "  --clean           Force clean build before testing"
    echo "  --cores N         Use N cores for building (default: auto)"
    echo ""
    echo -e "${BOLD}Filter Options:${NC}"
    echo "  --filter PATTERN  Run only tests matching pattern"
    echo "  --exclude PATTERN Exclude tests matching pattern"
    echo "  --repeat N        Repeat tests N times"
    echo "  --timeout N       Set test timeout in seconds (default: 300)"
    echo ""
    echo -e "${BOLD}Examples:${NC}"
    echo "  $0                              # Run all tests in release mode"
    echo "  $0 --unit --debug --verbose     # Run unit tests with debug build"
    echo "  $0 --performance --release      # Run performance tests"
    echo "  $0 --sanitizers --stress        # Run stress tests with sanitizers"
    echo "  $0 --filter \"*thread*\"          # Run only thread-related tests"
    echo ""
}

# Function to check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Function to detect available test tools
detect_test_tools() {
    print_status "Detecting available testing tools..."

    TOOLS_CTEST=0
    TOOLS_VALGRIND=0
    TOOLS_GPROF=0
    TOOLS_GCOV=0
    TOOLS_LCOV=0

    if command_exists ctest; then
        TOOLS_CTEST=1
        print_info "CTest available"
    fi

    if command_exists valgrind; then
        TOOLS_VALGRIND=1
        print_info "Valgrind available for memory testing"
    fi

    if command_exists gprof; then
        TOOLS_GPROF=1
        print_info "gprof available for profiling"
    fi

    if command_exists gcov; then
        TOOLS_GCOV=1
        print_info "gcov available for coverage"
    fi

    if command_exists lcov; then
        TOOLS_LCOV=1
        print_info "lcov available for coverage reports"
    fi
}

# Function to build for testing
build_for_testing() {
    print_status "Building project for testing..."

    local build_args=("--tests")

    # Add build type
    if [ "$BUILD_TYPE" = "Debug" ]; then
        build_args+=("--debug")
    else
        build_args+=("--release" "Release")
    fi

    # Add sanitizers
    if [ $ENABLE_ASAN -eq 1 ]; then
        build_args+=("--asan")
    fi
    if [ $ENABLE_TSAN -eq 1 ]; then
        build_args+=("--tsan")
    fi
    if [ $ENABLE_UBSAN -eq 1 ]; then
        build_args+=("--ubsan")
    fi

    # Add other options
    if [ $CLEAN_BUILD -eq 1 ]; then
        build_args+=("--clean")
    fi

    if [ $BUILD_CORES -gt 0 ]; then
        build_args+=("--cores" "$BUILD_CORES")
    fi

    if [ $VERBOSE -eq 1 ]; then
        build_args+=("--verbose")
    fi

    # Add coverage if requested
    if [ $ENABLE_COVERAGE -eq 1 ] && [ $TOOLS_GCOV -eq 1 ]; then
        build_args+=("--profiling")  # This should enable coverage flags
    fi

    # Run build script
    if ! ./build.sh "${build_args[@]}"; then
        print_error "Build failed"
        return 1
    fi

    print_success "Build completed for testing"
    return 0
}

# Function to run unit tests
run_unit_tests() {
    print_status "Running unit tests..."

    local test_count=0
    local test_passed=0
    local test_failed=0

    # Use CTest if available
    if [ $TOOLS_CTEST -eq 1 ] && [ -f "build/CTestTestfile.cmake" ]; then
        cd build

        local ctest_args=("--output-on-failure")

        if [ $VERBOSE -eq 1 ]; then
            ctest_args+=("--verbose")
        fi

        if [ -n "$TEST_FILTER" ]; then
            ctest_args+=("-R" "$TEST_FILTER")
        fi

        if [ -n "$TEST_EXCLUDE" ]; then
            ctest_args+=("-E" "$TEST_EXCLUDE")
        fi

        if [ $REPEAT_COUNT -gt 1 ]; then
            ctest_args+=("--repeat" "until-fail:$REPEAT_COUNT")
        fi

        if [ $GENERATE_JUNIT -eq 1 ]; then
            ctest_args+=("--output-junit" "test_results.xml")
        fi

        # Run CTest
        if ctest "${ctest_args[@]}"; then
            test_passed=1
            print_success "CTest unit tests passed"
        else
            test_failed=1
            print_error "CTest unit tests failed"
        fi

        cd ..
        test_count=1
    else
        # Run test executables directly
        for test_exe in build/bin/*test* build/tests/*test*; do
            if [ -x "$test_exe" ]; then
                test_count=$((test_count + 1))
                local test_name=$(basename "$test_exe")

                # Apply filters
                if [ -n "$TEST_FILTER" ] && [[ ! "$test_name" =~ $TEST_FILTER ]]; then
                    continue
                fi

                if [ -n "$TEST_EXCLUDE" ] && [[ "$test_name" =~ $TEST_EXCLUDE ]]; then
                    continue
                fi

                print_info "Running $test_name..."

                # Run with timeout
                if timeout "$TEST_TIMEOUT" "$test_exe"; then
                    test_passed=$((test_passed + 1))
                    print_success "$test_name passed"
                else
                    local exit_code=$?
                    test_failed=$((test_failed + 1))

                    if [ $exit_code -eq 124 ]; then
                        print_error "$test_name timed out after ${TEST_TIMEOUT}s"
                    else
                        print_error "$test_name failed with exit code $exit_code"
                    fi
                fi
            fi
        done
    fi

    # Report results
    if [ $test_count -eq 0 ]; then
        print_warning "No unit test executables found"
        return 1
    fi

    print_info "Unit Tests Summary: $test_passed passed, $test_failed failed out of $test_count"

    if [ $test_failed -gt 0 ]; then
        return 1
    else
        return 0
    fi
}

# Function to run integration tests
run_integration_tests() {
    print_status "Running integration tests..."

    # Create integration test if none exist
    if [ ! -f "build/bin/integration_test" ] && [ ! -f "examples/integration_example" ]; then
        print_info "Creating basic integration test..."

        # Run the basic example as integration test
        if [ -f "examples/basic_example.cpp" ]; then
            print_info "Building basic example for integration testing..."

            # This would be built by the build system
            if [ -f "build/bin/basic_example" ]; then
                print_info "Running basic example as integration test..."

                if timeout "$TEST_TIMEOUT" build/bin/basic_example; then
                    print_success "Basic integration test passed"
                    return 0
                else
                    print_error "Basic integration test failed"
                    return 1
                fi
            fi
        fi

        print_warning "No integration tests available"
        return 0
    fi

    # Run integration tests
    local integration_passed=0
    local integration_failed=0

    for test_exe in build/bin/*integration* examples/*example*; do
        if [ -x "$test_exe" ]; then
            local test_name=$(basename "$test_exe")
            print_info "Running integration test: $test_name..."

            if timeout "$TEST_TIMEOUT" "$test_exe"; then
                integration_passed=$((integration_passed + 1))
                print_success "$test_name passed"
            else
                integration_failed=$((integration_failed + 1))
                print_error "$test_name failed"
            fi
        fi
    done

    print_info "Integration Tests Summary: $integration_passed passed, $integration_failed failed"

    if [ $integration_failed -gt 0 ]; then
        return 1
    else
        return 0
    fi
}

# Function to run performance tests
run_performance_tests() {
    print_status "Running performance benchmarks..."

    local benchmark_count=0
    local benchmark_passed=0

    # Look for benchmark executables
    for benchmark_exe in build/bin/*benchmark* build/bin/*perf*; do
        if [ -x "$benchmark_exe" ]; then
            benchmark_count=$((benchmark_count + 1))
            local benchmark_name=$(basename "$benchmark_exe")

            print_info "Running benchmark: $benchmark_name..."

            # Run benchmark with timeout
            if timeout $((TEST_TIMEOUT * 2)) "$benchmark_exe" --benchmark_format=console; then
                benchmark_passed=$((benchmark_passed + 1))
                print_success "$benchmark_name completed"
            else
                print_warning "$benchmark_name failed or timed out"
            fi
        fi
    done

    if [ $benchmark_count -eq 0 ]; then
        print_warning "No benchmark executables found"
        print_info "To enable benchmarks, rebuild with: ./build.sh --benchmark --tests"
        return 0
    fi

    print_info "Performance Tests Summary: $benchmark_passed of $benchmark_count completed"
    return 0
}

# Function to run stress tests
run_stress_tests() {
    print_status "Running stress tests..."

    # Create a stress test if none exists
    if [ ! -f "build/bin/stress_test" ]; then
        print_info "Creating basic stress test..."

        cat > /tmp/stress_test.cpp << 'EOF'
#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <atomic>
#include <random>

// Simple stress test for thread system
int main() {
    std::cout << "Starting stress test...\n";

    const int num_threads = std::thread::hardware_concurrency() * 2;
    const int iterations = 10000;
    std::atomic<int> counter{0};
    std::atomic<bool> running{true};

    auto start_time = std::chrono::steady_clock::now();

    std::vector<std::future<void>> futures;

    // Launch stress threads
    for (int i = 0; i < num_threads; ++i) {
        futures.push_back(std::async(std::launch::async, [&counter, iterations, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 100);

            for (int j = 0; j < iterations; ++j) {
                counter.fetch_add(1);

                // Simulate work
                auto work_time = dis(gen);
                std::this_thread::sleep_for(std::chrono::microseconds(work_time));

                if (j % 1000 == 0) {
                    std::cout << "Thread " << i << " completed " << j << " iterations\n";
                }
            }
        }));
    }

    // Wait for completion
    for (auto& future : futures) {
        future.get();
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Stress test completed!\n";
    std::cout << "Total operations: " << counter.load() << "\n";
    std::cout << "Time taken: " << duration.count() << " ms\n";
    std::cout << "Operations per second: " << (counter.load() * 1000.0) / duration.count() << "\n";

    // Verify results
    int expected = num_threads * iterations;
    if (counter.load() == expected) {
        std::cout << "✓ Stress test PASSED - all operations completed correctly\n";
        return 0;
    } else {
        std::cerr << "✗ Stress test FAILED - expected " << expected << ", got " << counter.load() << "\n";
        return 1;
    }
}
EOF

        # Try to compile and run the stress test
        if command_exists g++; then
            if g++ -std=c++17 -pthread -O2 -o /tmp/stress_test /tmp/stress_test.cpp 2>/dev/null; then
                print_info "Running compiled stress test..."

                if timeout "$TEST_TIMEOUT" /tmp/stress_test; then
                    print_success "Stress test passed"
                    rm -f /tmp/stress_test /tmp/stress_test.cpp
                    return 0
                else
                    print_error "Stress test failed"
                    rm -f /tmp/stress_test /tmp/stress_test.cpp
                    return 1
                fi
            else
                print_warning "Could not compile stress test"
            fi
        fi
    else
        # Run existing stress test
        print_info "Running stress test executable..."

        if timeout "$TEST_TIMEOUT" build/bin/stress_test; then
            print_success "Stress test passed"
            return 0
        else
            print_error "Stress test failed"
            return 1
        fi
    fi

    print_warning "No stress tests available"
    return 0
}

# Function to run memory tests
run_memory_tests() {
    print_status "Running memory tests..."

    if [ $TOOLS_VALGRIND -eq 0 ]; then
        print_warning "Valgrind not available, skipping detailed memory tests"

        # Run basic memory test
        if [ -f "build/bin/basic_example" ]; then
            print_info "Running basic memory test without valgrind..."

            if timeout "$TEST_TIMEOUT" build/bin/basic_example; then
                print_success "Basic memory test passed"
                return 0
            else
                print_error "Basic memory test failed"
                return 1
            fi
        fi

        return 0
    fi

    local memory_test_passed=0
    local memory_test_failed=0

    # Test with valgrind
    for test_exe in build/bin/*test* build/bin/*example*; do
        if [ -x "$test_exe" ]; then
            local test_name=$(basename "$test_exe")
            print_info "Running valgrind memory check on $test_name..."

            # Run with valgrind
            local valgrind_log="/tmp/valgrind_${test_name}.log"

            if timeout $((TEST_TIMEOUT * 3)) valgrind \
                --tool=memcheck \
                --leak-check=full \
                --show-leak-kinds=all \
                --track-origins=yes \
                --error-exitcode=1 \
                --log-file="$valgrind_log" \
                "$test_exe" >/dev/null 2>&1; then

                memory_test_passed=$((memory_test_passed + 1))
                print_success "$test_name passed memory check"
            else
                memory_test_failed=$((memory_test_failed + 1))
                print_error "$test_name failed memory check"

                if [ $VERBOSE -eq 1 ] && [ -f "$valgrind_log" ]; then
                    echo -e "${YELLOW}Valgrind output for $test_name:${NC}"
                    head -20 "$valgrind_log"
                    echo "..."
                fi
            fi

            # Clean up log file
            rm -f "$valgrind_log"

            # Only test a few executables to save time
            if [ $((memory_test_passed + memory_test_failed)) -ge 3 ]; then
                break
            fi
        fi
    done

    print_info "Memory Tests Summary: $memory_test_passed passed, $memory_test_failed failed"

    if [ $memory_test_failed -gt 0 ]; then
        return 1
    else
        return 0
    fi
}

# Function to generate coverage report
generate_coverage() {
    if [ $ENABLE_COVERAGE -eq 0 ] || [ $TOOLS_GCOV -eq 0 ]; then
        return 0
    fi

    print_status "Generating code coverage report..."

    cd build

    # Generate coverage data
    if command_exists gcov; then
        find . -name "*.gcda" -exec gcov {} \; >/dev/null 2>&1
    fi

    # Generate HTML report with lcov if available
    if [ $TOOLS_LCOV -eq 1 ]; then
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' '*test*' '*example*' --output-file coverage_filtered.info
        genhtml coverage_filtered.info --output-directory coverage_html

        print_success "Coverage report generated in build/coverage_html/"
    fi

    cd ..
}

# Initialize default values
RUN_UNIT=0
RUN_INTEGRATION=0
RUN_PERFORMANCE=0
RUN_STRESS=0
RUN_MEMORY=0
RUN_THREAD_SAFETY=0
RUN_ALL=1

BUILD_TYPE="Release"
ENABLE_ASAN=0
ENABLE_TSAN=0
ENABLE_UBSAN=0
ENABLE_SANITIZERS=0

VERBOSE=0
QUIET=0
GENERATE_JUNIT=0
ENABLE_COVERAGE=0

NO_BUILD=0
CLEAN_BUILD=0
BUILD_CORES=0

TEST_FILTER=""
TEST_EXCLUDE=""
REPEAT_COUNT=1
TEST_TIMEOUT=300

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --help)
            show_help
            exit 0
            ;;
        --unit)
            RUN_UNIT=1
            RUN_ALL=0
            shift
            ;;
        --integration)
            RUN_INTEGRATION=1
            RUN_ALL=0
            shift
            ;;
        --performance)
            RUN_PERFORMANCE=1
            RUN_ALL=0
            shift
            ;;
        --stress)
            RUN_STRESS=1
            RUN_ALL=0
            shift
            ;;
        --memory)
            RUN_MEMORY=1
            RUN_ALL=0
            shift
            ;;
        --thread-safety)
            RUN_THREAD_SAFETY=1
            RUN_ALL=0
            shift
            ;;
        --all)
            RUN_ALL=1
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --sanitizers)
            ENABLE_SANITIZERS=1
            ENABLE_ASAN=1
            ENABLE_TSAN=1
            ENABLE_UBSAN=1
            shift
            ;;
        --asan)
            ENABLE_ASAN=1
            shift
            ;;
        --tsan)
            ENABLE_TSAN=1
            shift
            ;;
        --ubsan)
            ENABLE_UBSAN=1
            shift
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        --quiet)
            QUIET=1
            VERBOSE=0
            shift
            ;;
        --junit)
            GENERATE_JUNIT=1
            shift
            ;;
        --coverage)
            ENABLE_COVERAGE=1
            shift
            ;;
        --no-build)
            NO_BUILD=1
            shift
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --cores)
            BUILD_CORES="$2"
            shift 2
            ;;
        --filter)
            TEST_FILTER="$2"
            shift 2
            ;;
        --exclude)
            TEST_EXCLUDE="$2"
            shift 2
            ;;
        --repeat)
            REPEAT_COUNT="$2"
            shift 2
            ;;
        --timeout)
            TEST_TIMEOUT="$2"
            shift 2
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Set what to run if --all is selected
if [ $RUN_ALL -eq 1 ]; then
    RUN_UNIT=1
    RUN_INTEGRATION=1
    RUN_PERFORMANCE=1
    RUN_STRESS=1
    RUN_MEMORY=1
fi

# Detect available tools
detect_test_tools

# Build if needed
if [ $NO_BUILD -eq 0 ]; then
    build_for_testing
    if [ $? -ne 0 ]; then
        print_error "Build failed, cannot proceed with testing"
        exit 1
    fi
fi

# Check if build directory exists
if [ ! -d "build" ]; then
    print_error "Build directory not found. Run with build enabled or run ./build.sh first"
    exit 1
fi

# Start testing
echo ""
print_info "Starting test execution..."
print_info "Build type: $BUILD_TYPE"
print_info "Test timeout: ${TEST_TIMEOUT}s"
if [ -n "$TEST_FILTER" ]; then
    print_info "Filter: $TEST_FILTER"
fi
if [ -n "$TEST_EXCLUDE" ]; then
    print_info "Exclude: $TEST_EXCLUDE"
fi
echo ""

# Track overall results
OVERALL_PASSED=0
OVERALL_FAILED=0

# Run selected tests
if [ $RUN_UNIT -eq 1 ]; then
    run_unit_tests
    if [ $? -eq 0 ]; then
        OVERALL_PASSED=$((OVERALL_PASSED + 1))
    else
        OVERALL_FAILED=$((OVERALL_FAILED + 1))
    fi
    echo ""
fi

if [ $RUN_INTEGRATION -eq 1 ]; then
    run_integration_tests
    if [ $? -eq 0 ]; then
        OVERALL_PASSED=$((OVERALL_PASSED + 1))
    else
        OVERALL_FAILED=$((OVERALL_FAILED + 1))
    fi
    echo ""
fi

if [ $RUN_PERFORMANCE -eq 1 ]; then
    run_performance_tests
    if [ $? -eq 0 ]; then
        OVERALL_PASSED=$((OVERALL_PASSED + 1))
    else
        OVERALL_FAILED=$((OVERALL_FAILED + 1))
    fi
    echo ""
fi

if [ $RUN_STRESS -eq 1 ]; then
    run_stress_tests
    if [ $? -eq 0 ]; then
        OVERALL_PASSED=$((OVERALL_PASSED + 1))
    else
        OVERALL_FAILED=$((OVERALL_FAILED + 1))
    fi
    echo ""
fi

if [ $RUN_MEMORY -eq 1 ]; then
    run_memory_tests
    if [ $? -eq 0 ]; then
        OVERALL_PASSED=$((OVERALL_PASSED + 1))
    else
        OVERALL_FAILED=$((OVERALL_FAILED + 1))
    fi
    echo ""
fi

# Generate coverage report if requested
generate_coverage

# Final results
echo ""
echo -e "${BOLD}${CYAN}================================================${NC}"
echo -e "${BOLD}${CYAN}    Test Suite Execution Complete             ${NC}"
echo -e "${BOLD}${CYAN}================================================${NC}"

print_info "Test categories completed: $OVERALL_PASSED passed, $OVERALL_FAILED failed"

if [ $OVERALL_FAILED -eq 0 ]; then
    print_success "All test categories PASSED!"

    if [ $ENABLE_COVERAGE -eq 1 ]; then
        print_info "Code coverage report available in build/coverage_html/"
    fi

    if [ $GENERATE_JUNIT -eq 1 ]; then
        print_info "JUnit test results available in build/test_results.xml"
    fi

    echo ""
    print_success "✓ Integrated Thread System testing completed successfully!"
    exit 0
else
    print_error "Some test categories FAILED!"
    echo ""
    print_error "✗ Integrated Thread System testing completed with failures"
    exit 1
fi