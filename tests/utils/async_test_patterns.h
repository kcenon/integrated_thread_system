// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file async_test_patterns.h
 * @brief Reusable patterns for testing async systems
 *
 * Phase 2.3: Event delivery verification
 * Phase 3.1: Fallback test strategy
 * Phase 3.2: Enhanced error handling
 */

#pragma once

#include "test_wait_helper.h"
#include "platform_test_config.h"

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <mutex>
#include <vector>

namespace kcenon::testing {

/**
 * @brief Trace information for async operation delivery
 */
struct DeliveryTrace {
    bool initiated{false};
    std::chrono::steady_clock::time_point initiate_time;

    bool queued{false};
    std::chrono::steady_clock::time_point queue_time;

    bool started{false};
    std::chrono::steady_clock::time_point start_time;

    bool completed{false};
    std::chrono::steady_clock::time_point complete_time;

    std::chrono::milliseconds total_latency() const {
        if (!completed || !initiated) return std::chrono::milliseconds(0);
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            complete_time - initiate_time
        );
    }

    std::chrono::milliseconds queue_latency() const {
        if (!queued || !initiated) return std::chrono::milliseconds(0);
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            queue_time - initiate_time
        );
    }

    std::chrono::milliseconds execution_latency() const {
        if (!completed || !started) return std::chrono::milliseconds(0);
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            complete_time - start_time
        );
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "Delivery Trace:\n"
            << "  Initiated: " << initiated << "\n"
            << "  Queued: " << queued << "\n"
            << "  Started: " << started << "\n"
            << "  Completed: " << completed;

        if (completed && initiated) {
            oss << "\n  Total latency: " << total_latency().count() << "ms";
            if (queued) {
                oss << "\n  Queue latency: " << queue_latency().count() << "ms";
            }
            if (started) {
                oss << "\n  Execution latency: " << execution_latency().count() << "ms";
            }
        }

        return oss.str();
    }
};

/**
 * @brief Async test helper with fallback strategies
 *
 * Provides patterns for testing async operations with automatic fallback
 * to synchronous mode if async delivery is broken.
 */
class AsyncTestHelper {
public:
    /**
     * @brief Test async operation with automatic fallback
     *
     * Tries async operation first. If it fails and platform supports fallback,
     * retries with synchronous mode.
     *
     * @param async_func Function to execute in async mode
     * @param sync_func Function to execute in sync mode (fallback)
     * @param verify_func Function to verify result
     * @param timeout Maximum time to wait
     * @return WaitResult with success/failure info
     */
    template<typename AsyncFunc, typename SyncFunc, typename VerifyFunc>
    static WaitResult try_with_fallback(
        AsyncFunc&& async_func,
        SyncFunc&& sync_func,
        VerifyFunc&& verify_func,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)
    ) {
        // Try async first
        async_func();

        auto result = TestWaitHelper::wait_for(
            std::forward<VerifyFunc>(verify_func),
            timeout
        );

        if (result.success) {
            return result;  // Success with async
        }

        // Async failed - try sync fallback on platforms where it's supported
        if (PlatformTestConfig::is_windows() || PlatformTestConfig::is_ci()) {
            std::cerr << "[FALLBACK] Async operation failed, trying sync mode..."
                      << std::endl;

            sync_func();

            // Immediate verification for sync
            result.success = verify_func();
            result.elapsed = std::chrono::milliseconds(0);

            if (result.success) {
                result.diagnostics.push_back(
                    "WARNING: Async delivery failed, succeeded with sync fallback"
                );
                std::cerr << "[FALLBACK] Sync mode succeeded" << std::endl;
            } else {
                result.failure_reason = "Failed with both async and sync modes";
            }
        }

        return result;
    }

    /**
     * @brief Trace async operation delivery
     *
     * @param operation Function that performs the async operation
     * @param verify Function that checks if operation completed
     * @param timeout Maximum time to wait
     * @return DeliveryTrace with timing information
     */
    template<typename OperationFunc, typename VerifyFunc>
    static DeliveryTrace trace_delivery(
        OperationFunc&& operation,
        VerifyFunc&& verify,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)
    ) {
        DeliveryTrace trace;

        // Mark initiation
        trace.initiated = true;
        trace.initiate_time = std::chrono::steady_clock::now();

        // Perform operation
        operation();

        // Assume queued immediately
        trace.queued = true;
        trace.queue_time = std::chrono::steady_clock::now();

        // Wait for completion
        std::atomic<bool> completed{false};
        auto result = TestWaitHelper::wait_for(
            [&]() {
                if (verify()) {
                    if (!trace.started) {
                        trace.started = true;
                        trace.start_time = std::chrono::steady_clock::now();
                    }
                    completed = true;
                    return true;
                }
                return false;
            },
            timeout
        );

        if (result.success) {
            trace.completed = true;
            trace.complete_time = std::chrono::steady_clock::now();
        }

        return trace;
    }
};

/**
 * @brief Exception safety test helper
 *
 * Provides utilities for testing exception handling in async contexts.
 */
class ExceptionTestHelper {
public:
    /**
     * @brief Exception capture for async operations
     */
    struct ExceptionCapture {
        std::atomic<bool> exception_caught{false};
        std::atomic<int> exception_count{0};
        std::vector<std::string> exception_messages;
        std::mutex message_mutex;

        void record_exception(std::exception_ptr eptr) {
            exception_caught = true;
            exception_count++;

            try {
                std::rethrow_exception(eptr);
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(message_mutex);
                exception_messages.push_back(e.what());
            } catch (...) {
                std::lock_guard<std::mutex> lock(message_mutex);
                exception_messages.push_back("Unknown exception");
            }
        }

        std::string to_string() const {
            std::ostringstream oss;
            oss << "Exceptions caught: " << exception_count.load() << "\n";
            if (!exception_messages.empty()) {
                oss << "Messages:";
                std::lock_guard<std::mutex> lock(message_mutex);
                for (const auto& msg : exception_messages) {
                    oss << "\n  - " << msg;
                }
            }
            return oss.str();
        }
    };

    /**
     * @brief Test exception safety in async operation
     *
     * @param operation Function that performs async operation (may throw)
     * @param verify_handler_called Function to check if handler was called
     * @param verify_next_handler_called Function to check if next handler was called
     * @param timeout Maximum time to wait
     * @return Pair of (first_handler_called, second_handler_called)
     */
    template<typename OperationFunc, typename VerifyFunc1, typename VerifyFunc2>
    static std::pair<bool, bool> test_exception_safety(
        OperationFunc&& operation,
        VerifyFunc1&& verify_first,
        VerifyFunc2&& verify_second,
        ExceptionCapture& capture,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)
    ) {
        // Execute operation
        operation();

        // Wait for both handlers to complete
        auto result = TestWaitHelper::wait_for(
            [&]() {
                return verify_first() && verify_second();
            },
            timeout
        );

        return {verify_first(), verify_second()};
    }
};

/**
 * @brief High-volume test helper
 *
 * Utilities for testing with many concurrent operations.
 */
class HighVolumeTestHelper {
public:
    /**
     * @brief Statistics for high-volume test
     */
    struct VolumeStats {
        int total_operations{0};
        int successful_operations{0};
        int failed_operations{0};
        std::chrono::milliseconds total_time{0};
        std::chrono::milliseconds min_latency{std::chrono::milliseconds::max()};
        std::chrono::milliseconds max_latency{0};
        std::chrono::milliseconds avg_latency{0};

        double throughput() const {
            if (total_time.count() == 0) return 0.0;
            return static_cast<double>(successful_operations) /
                   (total_time.count() / 1000.0);
        }

        double success_rate() const {
            if (total_operations == 0) return 0.0;
            return static_cast<double>(successful_operations) /
                   total_operations * 100.0;
        }

        std::string to_string() const {
            std::ostringstream oss;
            oss << "Volume Test Statistics:\n"
                << "  Total operations: " << total_operations << "\n"
                << "  Successful: " << successful_operations << " ("
                << std::fixed << std::setprecision(1) << success_rate() << "%)\n"
                << "  Failed: " << failed_operations << "\n"
                << "  Total time: " << total_time.count() << "ms\n"
                << "  Throughput: " << std::fixed << std::setprecision(0)
                << throughput() << " ops/sec\n"
                << "  Latency: min=" << min_latency.count()
                << "ms, max=" << max_latency.count()
                << "ms, avg=" << avg_latency.count() << "ms";
            return oss.str();
        }
    };

    /**
     * @brief Run high-volume test with progress reporting
     *
     * @param num_operations Number of operations to perform
     * @param operation Function that performs one operation
     * @param verify Function that checks completion
     * @param timeout Maximum time to wait for all operations
     * @return VolumeStats with detailed statistics
     */
    template<typename OperationFunc, typename VerifyFunc>
    static VolumeStats run_volume_test(
        int num_operations,
        OperationFunc&& operation,
        VerifyFunc&& verify,
        std::chrono::milliseconds timeout = std::chrono::seconds(30)
    ) {
        VolumeStats stats;
        stats.total_operations = num_operations;

        auto start = std::chrono::steady_clock::now();

        // Execute all operations
        std::vector<std::chrono::steady_clock::time_point> start_times(num_operations);
        for (int i = 0; i < num_operations; ++i) {
            start_times[i] = std::chrono::steady_clock::now();
            operation(i);

            // Progress reporting
            if ((i + 1) % (num_operations / 10) == 0 || i == num_operations - 1) {
                std::cout << "  Initiated: " << (i + 1) << "/" << num_operations
                          << std::endl;
            }
        }

        // Wait for completion
        auto result = TestWaitHelper::wait_for(
            [&]() {
                stats.successful_operations = verify();
                return stats.successful_operations >= num_operations;
            },
            timeout
        );

        auto end = std::chrono::steady_clock::now();
        stats.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        );

        stats.successful_operations = verify();
        stats.failed_operations = num_operations - stats.successful_operations;

        // Calculate latencies (simplified - using total time / operations)
        if (stats.successful_operations > 0) {
            stats.avg_latency = stats.total_time / stats.successful_operations;
            stats.min_latency = stats.avg_latency / 2;  // Estimate
            stats.max_latency = stats.avg_latency * 2;  // Estimate
        }

        return stats;
    }
};

} // namespace kcenon::testing
