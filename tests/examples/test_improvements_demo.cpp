/**
 * @file test_improvements_demo.cpp
 * @brief Demonstration of all Phase 1-3 test improvements
 *
 * This file showcases how to use the new test infrastructure to write
 * robust, maintainable, and platform-aware tests.
 */

#include <gtest/gtest.h>
#include <kcenon/integrated/unified_thread_system.h>

// Phase 1-3 improvements
#include "../utils/test_wait_helper.h"
#include "../utils/platform_test_config.h"
#include "../utils/test_fixture_base.h"
#include "../utils/async_test_patterns.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace kcenon::integrated;
using namespace kcenon::testing;
using namespace std::chrono_literals;

// ============================================================================
// Demo 1: Basic Wait Helpers (Phase 1.3)
// ============================================================================

TEST(ImprovementsDemo, Phase1_WaitHelpers) {
    std::cout << "\n=== Phase 1.3: Enhanced Wait Mechanisms ===\n" << std::endl;

    unified_thread_system system;
    std::atomic<bool> task_completed{false};

    // Submit async task
    system.submit([&]() {
        std::this_thread::sleep_for(100ms);
        task_completed = true;
    });

    // ✅ NEW: Use TestWaitHelper instead of sleep
    auto result = TestWaitHelper::wait_for(
        [&]() { return task_completed.load(); },
        std::chrono::seconds(2)
    );

    // ✅ Rich diagnostic information
    ASSERT_TRUE(result)
        << "Task did not complete\n"
        << result.to_string();

    std::cout << "✓ Task completed successfully in "
              << result.elapsed.count() << "ms" << std::endl;
}

TEST(ImprovementsDemo, Phase1_WaitWithDiagnostics) {
    std::cout << "\n=== Phase 1.3: Wait with Custom Diagnostics ===\n" << std::endl;

    unified_thread_system system;
    std::atomic<int> counter{0};
    const int target = 10;

    // Submit multiple tasks
    for (int i = 0; i < target; ++i) {
        system.submit([&, i]() {
            std::this_thread::sleep_for(10ms * i);  // Variable delay
            counter++;
        });
    }

    // ✅ Wait with diagnostic function
    auto result = TestWaitHelper::wait_with_diagnostics(
        [&]() { return counter.load() >= target; },
        [&]() {
            std::vector<std::string> diags;
            diags.push_back("Current count: " + std::to_string(counter.load()) +
                           "/" + std::to_string(target));
            diags.push_back("Progress: " +
                           std::to_string(counter.load() * 100 / target) + "%");
            return diags;
        },
        std::chrono::seconds(2)
    );

    ASSERT_TRUE(result) << result.to_string();

    std::cout << "✓ All " << target << " tasks completed in "
              << result.elapsed.count() << "ms" << std::endl;
}

// ============================================================================
// Demo 2: Platform-Aware Configuration (Phase 2.1)
// ============================================================================

TEST(ImprovementsDemo, Phase2_PlatformAwareness) {
    std::cout << "\n=== Phase 2.1: Platform-Aware Configuration ===\n" << std::endl;

    // ✅ Print platform configuration
    std::cout << PlatformTestConfig::configuration_info() << "\n" << std::endl;

    // ✅ Use platform-appropriate timeouts
    auto timeout = PlatformTestConfig::event_delivery_timeout();
    std::cout << "Using timeout: " << timeout.count() << "ms for "
              << PlatformTestConfig::platform_name() << std::endl;

    // ✅ Adjust test size based on environment
    int num_tasks = PlatformTestConfig::is_ci() ? 100 : 500;
    std::cout << "Running with " << num_tasks << " tasks ("
              << (PlatformTestConfig::is_ci() ? "CI" : "Local") << " mode)"
              << std::endl;

    unified_thread_system system;
    std::atomic<int> completed{0};

    for (int i = 0; i < num_tasks; ++i) {
        system.submit([&]() {
            std::this_thread::sleep_for(1ms);
            completed++;
        });
    }

    auto result = TestWaitHelper::wait_for(
        [&]() { return completed.load() >= num_tasks; },
        PlatformTestConfig::adjusted_timeout(std::chrono::seconds(5))
    );

    ASSERT_TRUE(result) << result.to_string();
    std::cout << "✓ Completed " << num_tasks << " tasks in "
              << result.elapsed.count() << "ms" << std::endl;
}

// ============================================================================
// Demo 3: Test Fixtures (Phase 2.2)
// ============================================================================

class DemoTestFixture : public ThreadSystemTestFixture<unified_thread_system> {
protected:
    void SetUpSystem() override {
        std::cout << "  [Custom setup for " << test_name() << "]" << std::endl;
    }

    void TearDownSystem() override {
        std::cout << "  [Custom teardown for " << test_name() << "]" << std::endl;
    }

    void print_diagnostics() override {
        Base::print_diagnostics();
        std::cerr << "[DIAGNOSTICS] Custom diagnostic info" << std::endl;
    }
};

TEST_F(DemoTestFixture, Phase2_AutomaticSetupTeardown) {
    std::cout << "\n=== Phase 2.2: Test Fixture with Automatic Setup ===\n" << std::endl;

    // System is already created and ready (done in SetUp())
    ASSERT_NE(system_, nullptr);

    std::atomic<int> counter{0};

    // ✅ Use fixture's wait helper
    auto result = submit_and_wait(50, [&](int i) {
        counter++;
    });

    ASSERT_TRUE(result) << result.to_string();
    std::cout << "✓ Fixture test completed in "
              << result.elapsed.count() << "ms" << std::endl;
}

TEST_F(DemoTestFixture, Phase2_ThroughputMeasurement) {
    std::cout << "\n=== Phase 2.2: Built-in Throughput Measurement ===\n" << std::endl;

    // ✅ Use fixture's throughput measurement
    double throughput = measure_throughput(100, [](int i) {
        // Minimal work
        volatile int x = i * 2;
        (void)x;
    });

    ASSERT_GT(throughput, 0.0);

    print_throughput("Simple tasks", 100, throughput);
}

// ============================================================================
// Demo 4: Async Patterns (Phase 2.3 & 3.1)
// ============================================================================

TEST(ImprovementsDemo, Phase2_DeliveryTrace) {
    std::cout << "\n=== Phase 2.3: Delivery Trace ===\n" << std::endl;

    unified_thread_system system;
    std::atomic<bool> completed{false};

    // ✅ Trace the delivery
    auto trace = AsyncTestHelper::trace_delivery(
        [&]() {
            // Operation: submit task
            system.submit([&]() {
                std::this_thread::sleep_for(50ms);
                completed = true;
            });
        },
        [&]() {
            // Verify: check if completed
            return completed.load();
        },
        std::chrono::seconds(2)
    );

    ASSERT_TRUE(trace.completed) << trace.to_string();

    std::cout << trace.to_string() << std::endl;
    std::cout << "✓ Operation traced successfully" << std::endl;
}

TEST(ImprovementsDemo, Phase3_FallbackStrategy) {
    std::cout << "\n=== Phase 3.1: Fallback Strategy ===\n" << std::endl;

    unified_thread_system system;
    std::atomic<bool> completed{false};

    // ✅ Try with automatic fallback
    auto result = AsyncTestHelper::try_with_fallback(
        // Async operation
        [&]() {
            system.submit([&]() {
                std::this_thread::sleep_for(50ms);
                completed = true;
            });
        },
        // Sync fallback (if async fails)
        [&]() {
            // In this demo, just set the flag directly
            completed = true;
        },
        // Verification
        [&]() {
            return completed.load();
        },
        std::chrono::seconds(2)
    );

    ASSERT_TRUE(result) << result.to_string();

    if (!result.diagnostics.empty()) {
        std::cout << "⚠ Used fallback strategy:" << std::endl;
        for (const auto& diag : result.diagnostics) {
            std::cout << "  " << diag << std::endl;
        }
    } else {
        std::cout << "✓ Async operation succeeded without fallback" << std::endl;
    }
}

// ============================================================================
// Demo 5: Exception Safety (Phase 3.2)
// ============================================================================

TEST(ImprovementsDemo, Phase3_ExceptionSafety) {
    std::cout << "\n=== Phase 3.2: Exception Safety Testing ===\n" << std::endl;

    unified_thread_system system;

    std::atomic<bool> first_handler_called{false};
    std::atomic<bool> second_handler_called{false};

    // ✅ Use exception capture
    ExceptionTestHelper::ExceptionCapture capture;

    // First task throws
    system.submit([&]() {
        first_handler_called = true;
        throw std::runtime_error("Test exception");
    });

    // Second task should still execute
    system.submit([&]() {
        std::this_thread::sleep_for(10ms);
        second_handler_called = true;
    });

    // Wait for both
    auto result = TestWaitHelper::wait_for(
        [&]() {
            return first_handler_called.load() && second_handler_called.load();
        },
        std::chrono::seconds(2)
    );

    // Note: unified_thread_system doesn't propagate exceptions to other tasks,
    // so second task should complete regardless
    ASSERT_TRUE(second_handler_called.load())
        << "Second handler should have been called despite first handler's exception";

    std::cout << "✓ Exception safety verified" << std::endl;
    std::cout << "  First handler (threw): " << first_handler_called.load() << std::endl;
    std::cout << "  Second handler: " << second_handler_called.load() << std::endl;
}

// ============================================================================
// Demo 6: High-Volume Testing (Phase 2.3)
// ============================================================================

TEST(ImprovementsDemo, Phase2_HighVolumeTest) {
    std::cout << "\n=== Phase 2.3: High-Volume Testing ===\n" << std::endl;

    if (!PlatformTestConfig::run_high_volume_tests()) {
        GTEST_SKIP() << "High-volume tests disabled. "
                     << "Set RUN_HIGH_VOLUME_TESTS=1 to enable.";
    }

    unified_thread_system system;
    const int num_ops = PlatformTestConfig::high_volume_event_count();

    std::atomic<int> completed{0};

    std::cout << "Running high-volume test with " << num_ops << " operations..."
              << std::endl;

    // ✅ Use high-volume test helper
    auto stats = HighVolumeTestHelper::run_volume_test(
        num_ops,
        [&](int i) {
            // Operation: submit task
            system.submit([&, i]() {
                volatile int x = i * 2;
                (void)x;
                completed++;
            });
        },
        [&]() {
            // Verify: return count of completed
            return completed.load();
        },
        std::chrono::seconds(30)
    );

    std::cout << "\n" << stats.to_string() << std::endl;

    ASSERT_EQ(stats.successful_operations, num_ops)
        << "Not all operations completed successfully";
    ASSERT_GT(stats.throughput(), 1000.0)
        << "Throughput below minimum threshold";

    std::cout << "✓ High-volume test passed" << std::endl;
}

// ============================================================================
// Demo 7: Complete Example (All Phases)
// ============================================================================

class CompleteExampleTest : public ThreadSystemTestFixture<unified_thread_system> {
protected:
    void print_diagnostics() override {
        Base::print_diagnostics();
        std::cerr << "[DIAGNOSTICS] Completed tasks: " << completed_tasks_.load()
                  << "/" << total_tasks_ << std::endl;
    }

    std::atomic<int> completed_tasks_{0};
    int total_tasks_{0};
};

TEST_F(CompleteExampleTest, AllPhasesIntegrated) {
    std::cout << "\n=== Complete Example: All Phases Integrated ===\n" << std::endl;

    // Phase 2.1: Platform-aware configuration
    total_tasks_ = PlatformTestConfig::is_ci() ? 100 : 500;
    std::cout << "Task count (platform-aware): " << total_tasks_ << std::endl;

    // Phase 2.2: Use fixture's system (auto-setup)
    ASSERT_NE(system_, nullptr);

    // Phase 2.3: Trace first operation
    auto trace = AsyncTestHelper::trace_delivery(
        [&]() {
            system_->submit([&]() {
                std::this_thread::sleep_for(10ms);
                completed_tasks_++;
            });
        },
        [&]() { return completed_tasks_.load() > 0; },
        PlatformTestConfig::event_delivery_timeout()
    );

    ASSERT_TRUE(trace.completed) << "First task delivery trace:\n" << trace.to_string();
    std::cout << "✓ First task traced: " << trace.total_latency().count() << "ms latency"
              << std::endl;

    // Submit remaining tasks
    for (int i = 1; i < total_tasks_; ++i) {
        system_->submit([&, i]() {
            if (i % 10 == 0) {
                std::this_thread::sleep_for(5ms);
            }
            completed_tasks_++;
        });
    }

    // Phase 1.3 & 2.1: Wait with diagnostics and platform timeout
    auto result = wait_with_diagnostics(
        [&]() { return completed_tasks_.load() >= total_tasks_; },
        [&]() {
            std::vector<std::string> diags;
            diags.push_back("Progress: " + std::to_string(completed_tasks_.load()) +
                           "/" + std::to_string(total_tasks_));
            diags.push_back("Completion: " +
                           std::to_string(completed_tasks_.load() * 100 / total_tasks_) +
                           "%");
            return diags;
        }
    );

    // Phase 1.3: Rich assertion message
    ASSERT_TRUE(result)
        << "Task completion failed\n" << result.to_string();

    // Calculate and verify throughput
    double throughput = static_cast<double>(total_tasks_) /
                       (result.elapsed.count() / 1000.0);

    std::cout << "\n✓ All phases working together:" << std::endl;
    std::cout << "  - Completed: " << completed_tasks_.load() << "/" << total_tasks_
              << std::endl;
    std::cout << "  - Time: " << result.elapsed.count() << "ms" << std::endl;
    std::cout << "  - Throughput: " << throughput << " tasks/sec" << std::endl;
    std::cout << "  - Platform: " << PlatformTestConfig::platform_name() << std::endl;
    std::cout << "  - Environment: " << (PlatformTestConfig::is_ci() ? "CI" : "Local")
              << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "\n"
              << "╔════════════════════════════════════════════════════════════╗\n"
              << "║  Test Infrastructure Improvements Demo (Phase 1-3)        ║\n"
              << "╚════════════════════════════════════════════════════════════╝\n"
              << std::endl;

    std::cout << PlatformTestConfig::configuration_info() << "\n" << std::endl;

    int result = RUN_ALL_TESTS();

    std::cout << "\n"
              << "╔════════════════════════════════════════════════════════════╗\n"
              << "║  Demo Complete                                             ║\n"
              << "╚════════════════════════════════════════════════════════════╝\n"
              << std::endl;

    return result;
}
