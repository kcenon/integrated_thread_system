/**
 * @file test_basic_operations_improved.cpp
 * @brief Improved unit tests using new test infrastructure
 *
 * Demonstrates Phase 1-2 improvements:
 * - Enhanced wait mechanisms
 * - Platform-aware configuration
 * - Test fixtures with diagnostics
 */

#include <gtest/gtest.h>
#include <kcenon/integrated/unified_thread_system.h>
#include "../utils/test_fixture_base.h"
#include "../utils/test_wait_helper.h"
#include "../utils/platform_test_config.h"

#include <chrono>
#include <vector>
#include <atomic>
#include <numeric>
#include <thread>

using namespace kcenon::integrated;
using namespace kcenon::testing;
using namespace std::chrono_literals;

/**
 * @brief Improved test fixture with automatic diagnostics
 */
class ImprovedBasicOperationsTest : public ThreadSystemTestFixture<unified_thread_system> {
protected:
    void print_diagnostics() override {
        Base::print_diagnostics();
        std::cerr << "[DIAGNOSTICS] Test threads: "
                  << PlatformTestConfig::test_thread_count() << std::endl;
    }
};

// ============================================================================
// Basic Operations Tests with Improved Infrastructure
// ============================================================================

TEST_F(ImprovedBasicOperationsTest, SystemCreation) {
    ASSERT_NE(system_, nullptr);
}

TEST_F(ImprovedBasicOperationsTest, SimpleTaskSubmission) {
    auto future = system_->submit([]() {
        return 42;
    });

    EXPECT_EQ(future.get(), 42);
}

TEST_F(ImprovedBasicOperationsTest, VoidTaskWithWait) {
    std::atomic<bool> executed{false};

    system_->submit([&executed]() {
        executed = true;
    });

    // ✅ NEW: Use wait helper instead of future.wait()
    auto result = wait_for([&]() { return executed.load(); });

    ASSERT_TRUE(result)
        << "Task not executed: " << result.to_string();
}

TEST_F(ImprovedBasicOperationsTest, MultipleTasksWithProgress) {
    const int num_tasks = PlatformTestConfig::is_ci() ? 50 : 100;
    std::atomic<int> completed{0};

    std::cout << "Submitting " << num_tasks << " tasks..." << std::endl;

    for (int i = 0; i < num_tasks; ++i) {
        system_->submit([&, i]() {
            // Simulate work
            std::this_thread::sleep_for(1ms);
            completed++;

            if (completed % 25 == 0) {
                std::cout << "  Progress: " << completed.load()
                          << "/" << num_tasks << std::endl;
            }
        });
    }

    // ✅ NEW: Wait with platform-appropriate timeout
    auto result = wait_for_count(completed, num_tasks);

    ASSERT_TRUE(result)
        << "Not all tasks completed: " << completed.load()
        << "/" << num_tasks << "\n" << result.to_string();

    EXPECT_EQ(completed.load(), num_tasks);

    std::cout << "All tasks completed in " << result.elapsed.count()
              << "ms" << std::endl;
}

TEST_F(ImprovedBasicOperationsTest, ExceptionPropagationWithDiagnostics) {
    auto future = system_->submit([]() -> int {
        throw std::runtime_error("Test exception");
        return 0;  // Never reached
    });

    // ✅ NEW: Better exception testing
    bool exception_caught = false;
    std::string exception_message;

    try {
        future.get();
    } catch (const std::runtime_error& e) {
        exception_caught = true;
        exception_message = e.what();
    }

    ASSERT_TRUE(exception_caught)
        << "Expected exception was not thrown";
    EXPECT_EQ(exception_message, "Test exception");

    std::cout << "Exception correctly propagated: " << exception_message
              << std::endl;
}

TEST_F(ImprovedBasicOperationsTest, ConcurrentExecutionWithMetrics) {
    const int num_tasks = 100;
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};
    std::atomic<int> completed{0};

    for (int i = 0; i < num_tasks; ++i) {
        system_->submit([&]() {
            // Track concurrency
            int current = concurrent_count.fetch_add(1) + 1;

            int expected = max_concurrent.load();
            while (current > expected &&
                   !max_concurrent.compare_exchange_weak(expected, current)) {
            }

            std::this_thread::sleep_for(10ms);
            concurrent_count.fetch_sub(1);
            completed++;
        });
    }

    // ✅ NEW: Wait with diagnostics
    auto result = wait_with_diagnostics(
        [&]() { return completed.load() >= num_tasks; },
        [&]() {
            std::vector<std::string> diags;
            diags.push_back("Completed: " + std::to_string(completed.load()) +
                           "/" + std::to_string(num_tasks));
            diags.push_back("Max concurrent: " +
                           std::to_string(max_concurrent.load()));
            diags.push_back("Current concurrent: " +
                           std::to_string(concurrent_count.load()));
            return diags;
        }
    );

    ASSERT_TRUE(result)
        << "Concurrent execution failed\n" << result.to_string();

    EXPECT_GT(max_concurrent.load(), 1)
        << "Expected concurrent execution, got sequential";

    std::cout << "Max concurrent tasks: " << max_concurrent.load() << std::endl;
    std::cout << "Completed in: " << result.elapsed.count() << "ms" << std::endl;
}

// ============================================================================
// Performance Tests with Platform Awareness
// ============================================================================

TEST_F(ImprovedBasicOperationsTest, ThroughputMeasurement) {
    if (!PlatformTestConfig::run_high_volume_tests()) {
        GTEST_SKIP() << "High volume tests disabled in CI. "
                     << "Set RUN_HIGH_VOLUME_TESTS=1 to enable.";
    }

    const int num_tasks = PlatformTestConfig::high_volume_event_count();

    std::cout << "Measuring throughput with " << num_tasks << " tasks..."
              << std::endl;

    // Measure throughput
    double throughput = measure_throughput(num_tasks, [](int i) {
        // Minimal work per task
        volatile int x = i * 2;
        (void)x;
    });

    ASSERT_GT(throughput, 0.0) << "Throughput measurement failed";

    print_throughput("Simple tasks", num_tasks, throughput);

    // ✅ NEW: Platform-aware expectations
    double expected_throughput = PlatformTestConfig::expected_task_throughput();
    double threshold = expected_throughput * 0.1;  // 10% of expected

    EXPECT_GT(throughput, threshold)
        << "Throughput " << throughput << " tasks/sec is below threshold "
        << threshold << " (expected ~" << expected_throughput << ")";
}

TEST_F(ImprovedBasicOperationsTest, StressTestWithAutomaticScaling) {
    // ✅ NEW: Scale test based on platform
    const int num_tasks = PlatformTestConfig::is_ci() ? 500 : 1000;
    const auto timeout = PlatformTestConfig::is_ci()
        ? std::chrono::seconds(30)
        : std::chrono::seconds(10);

    std::cout << "Stress test: " << num_tasks << " tasks" << std::endl;

    std::atomic<int> completed{0};
    std::vector<int> results(num_tasks);

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < num_tasks; ++i) {
        system_->submit([&, i]() {
            results[i] = i % 100;
            completed++;
        });
    }

    // Wait with appropriate timeout
    auto result = wait_for(
        [&]() { return completed.load() >= num_tasks; },
        timeout
    );

    ASSERT_TRUE(result)
        << "Stress test failed: " << completed.load()
        << "/" << num_tasks << " completed\n"
        << result.to_string();

    // Verify all results
    for (int i = 0; i < num_tasks; ++i) {
        EXPECT_EQ(results[i], i % 100);
    }

    std::cout << "Stress test completed: " << num_tasks
              << " tasks in " << result.elapsed.count() << "ms" << std::endl;
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST(ImprovedConfigurationTest, CustomWorkerCountWithPlatformAwareness) {
    unified_thread_system::config cfg;
    cfg.thread_count = PlatformTestConfig::test_thread_count();

    std::cout << "Using " << cfg.thread_count << " worker threads" << std::endl;

    unified_thread_system system(cfg);

    std::atomic<int> completed{0};
    const int num_tasks = cfg.thread_count * 4;

    for (int i = 0; i < num_tasks; ++i) {
        system.submit([&, i]() {
            std::this_thread::sleep_for(10ms);
            completed++;
            return i;
        });
    }

    // Wait with platform timeout
    auto result = TestWaitHelper::wait_for_count(
        completed,
        num_tasks,
        PlatformTestConfig::adjusted_timeout(std::chrono::seconds(5))
    );

    ASSERT_TRUE(result)
        << "Tasks did not complete\n" << result.to_string();

    std::cout << "Completed " << num_tasks << " tasks on "
              << cfg.thread_count << " threads in "
              << result.elapsed.count() << "ms" << std::endl;
}

// ============================================================================
// Diagnostic Test (Demonstrates Failure Reporting)
// ============================================================================

TEST_F(ImprovedBasicOperationsTest, DISABLED_DemonstrateDiagnosticOutput) {
    // This test is disabled but demonstrates the diagnostic output
    // Remove DISABLED_ to see rich failure information

    std::atomic<int> counter{0};

    system_->submit([&]() {
        // Intentionally never increment counter to demonstrate timeout
        std::this_thread::sleep_for(10s);
    });

    // This will timeout and show rich diagnostics
    auto result = wait_with_diagnostics(
        [&]() { return counter.load() > 0; },
        [&]() {
            std::vector<std::string> diags;
            diags.push_back("Counter value: " + std::to_string(counter.load()));
            diags.push_back("Expected: > 0");
            diags.push_back("Platform: " + std::string(PlatformTestConfig::platform_name()));
            diags.push_back("Timeout used: " +
                           std::to_string(PlatformTestConfig::event_delivery_timeout().count()) +
                           "ms");
            return diags;
        }
    );

    ASSERT_TRUE(result)
        << "This will fail with rich diagnostic output\n"
        << result.to_string();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Print configuration at startup
    std::cout << "\n" << PlatformTestConfig::configuration_info()
              << "\n" << std::endl;

    return RUN_ALL_TESTS();
}
