// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file test_fixture_base.h
 * @brief Base test fixture with automatic diagnostics and health checks
 *
 * Phase 2.2: Test fixtures with automatic diagnostics
 */

#pragma once

#include "platform_test_config.h"
#include "test_wait_helper.h"

#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <memory>

namespace kcenon::testing {

/**
 * @brief Base test fixture with automatic setup/teardown and diagnostics
 *
 * Provides:
 * - Automatic timing of tests
 * - Platform-aware configuration
 * - Diagnostic logging on failure
 * - Helper methods for waiting
 *
 * Usage:
 * @code
 * class MyTest : public TestFixtureBase<unified_thread_system> {
 * protected:
 *     void SetUpSystem() override {
 *         // Custom setup for your system
 *     }
 * };
 *
 * TEST_F(MyTest, SomeTest) {
 *     // Test code
 *     auto result = wait_for([&]() { return condition; });
 *     ASSERT_TRUE(result) << result.to_string();
 * }
 * @endcode
 */
template<typename SystemType>
class TestFixtureBase : public ::testing::Test {
protected:
    /**
     * @brief Setup called before each test
     */
    void SetUp() override {
        test_start_time_ = std::chrono::steady_clock::now();

        // Print platform info for first test
        static bool printed_config = false;
        if (!printed_config) {
            std::cout << "\n" << PlatformTestConfig::configuration_info()
                      << "\n" << std::endl;
            printed_config = true;
        }

        std::cout << "[SETUP] " << test_name() << std::endl;

        // Create system
        system_ = std::make_unique<SystemType>();

        // Call custom setup
        SetUpSystem();

        std::cout << "[SETUP] " << test_name() << " ready" << std::endl;
    }

    /**
     * @brief Teardown called after each test
     */
    void TearDown() override {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - test_start_time_
        );

        std::cout << "[TEARDOWN] " << test_name()
                  << " completed in " << elapsed.count() << "ms" << std::endl;

        // Call custom teardown
        TearDownSystem();

        // Cleanup system
        system_.reset();

        // Check if test failed
        if (::testing::Test::HasFailure()) {
            std::cerr << "[TEARDOWN] Test FAILED: " << test_name() << std::endl;
            print_diagnostics();
        }
    }

    /**
     * @brief Custom setup hook for derived classes
     */
    virtual void SetUpSystem() {}

    /**
     * @brief Custom teardown hook for derived classes
     */
    virtual void TearDownSystem() {}

    /**
     * @brief Print diagnostic information (called on test failure)
     */
    virtual void print_diagnostics() {
        std::cerr << "[DIAGNOSTICS] Platform: " << PlatformTestConfig::platform_name()
                  << std::endl;
        std::cerr << "[DIAGNOSTICS] CI: " << (PlatformTestConfig::is_ci() ? "Yes" : "No")
                  << std::endl;
    }

    /**
     * @brief Get current test name
     */
    std::string test_name() const {
        const ::testing::TestInfo* test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();
        if (test_info) {
            return std::string(test_info->test_suite_name()) + "." +
                   test_info->name();
        }
        return "Unknown";
    }

    /**
     * @brief Wait for condition with platform-appropriate timeout
     */
    template<typename Predicate>
    WaitResult wait_for(Predicate&& pred) {
        return TestWaitHelper::wait_for(
            std::forward<Predicate>(pred),
            PlatformTestConfig::adjusted_timeout(
                PlatformTestConfig::event_delivery_timeout()
            ),
            PlatformTestConfig::poll_interval()
        );
    }

    /**
     * @brief Wait for condition with custom timeout
     */
    template<typename Predicate>
    WaitResult wait_for(Predicate&& pred, std::chrono::milliseconds timeout) {
        return TestWaitHelper::wait_for(
            std::forward<Predicate>(pred),
            PlatformTestConfig::adjusted_timeout(timeout),
            PlatformTestConfig::poll_interval()
        );
    }

    /**
     * @brief Wait for condition with diagnostics
     */
    template<typename Predicate, typename DiagnosticFunc>
    WaitResult wait_with_diagnostics(Predicate&& pred, DiagnosticFunc&& diag_func) {
        return TestWaitHelper::wait_with_diagnostics(
            std::forward<Predicate>(pred),
            std::forward<DiagnosticFunc>(diag_func),
            PlatformTestConfig::adjusted_timeout(
                PlatformTestConfig::event_delivery_timeout()
            ),
            PlatformTestConfig::poll_interval()
        );
    }

    /**
     * @brief Wait for counter to reach expected value
     */
    template<typename T>
    WaitResult wait_for_count(const std::atomic<T>& counter, T expected) {
        return TestWaitHelper::wait_for_count(
            counter,
            expected,
            PlatformTestConfig::adjusted_timeout(
                PlatformTestConfig::event_delivery_timeout()
            )
        );
    }

    // System under test
    std::unique_ptr<SystemType> system_;

private:
    std::chrono::steady_clock::time_point test_start_time_;
};

/**
 * @brief Thread system test fixture with progress reporting
 *
 * Specialized fixture for testing thread systems with additional
 * helper methods for common thread testing patterns.
 */
template<typename ThreadSystemType>
class ThreadSystemTestFixture : public TestFixtureBase<ThreadSystemType> {
protected:
    using Base = TestFixtureBase<ThreadSystemType>;

    /**
     * @brief Submit multiple tasks and wait for completion
     */
    template<typename Func>
    WaitResult submit_and_wait(int num_tasks, Func&& task_func) {
        std::atomic<int> completed{0};
        std::vector<std::future<void>> futures;

        for (int i = 0; i < num_tasks; ++i) {
            futures.push_back(Base::system_->submit([&, i]() {
                task_func(i);
                completed++;

                // Progress reporting for large batches
                if (num_tasks >= 100 && completed % (num_tasks / 10) == 0) {
                    std::cout << "  Progress: " << completed.load()
                              << "/" << num_tasks << std::endl;
                }
            }));
        }

        return Base::wait_for_count(completed, num_tasks);
    }

    /**
     * @brief Measure task throughput
     */
    template<typename Func>
    double measure_throughput(int num_tasks, Func&& task_func) {
        auto result = submit_and_wait(num_tasks, std::forward<Func>(task_func));

        if (!result.success) {
            return 0.0;
        }

        // Calculate throughput (tasks per second)
        double elapsed_sec = result.elapsed.count() / 1000.0;
        return num_tasks / elapsed_sec;
    }

    /**
     * @brief Print throughput result
     */
    void print_throughput(const std::string& test_name, int num_tasks, double throughput) {
        std::cout << "[PERF] " << test_name << ": "
                  << num_tasks << " tasks in "
                  << (num_tasks / throughput * 1000.0) << "ms ("
                  << throughput << " tasks/sec)" << std::endl;
    }
};

} // namespace kcenon::testing
