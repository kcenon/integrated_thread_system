// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file test_wait_helper.h
 * @brief Helper utilities for waiting on async conditions in tests
 *
 * Phase 1.3: Enhanced wait mechanisms with diagnostics
 */

#pragma once

#include <chrono>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace kcenon::testing {

/**
 * @brief Result of a wait operation with diagnostic information
 */
struct WaitResult {
    bool success{false};
    std::chrono::milliseconds elapsed{0};
    std::string failure_reason;
    std::vector<std::string> diagnostics;

    /**
     * @brief Convert result to human-readable string
     */
    std::string to_string() const {
        std::ostringstream oss;
        oss << "Wait " << (success ? "succeeded" : "FAILED")
            << " after " << elapsed.count() << "ms";

        if (!failure_reason.empty()) {
            oss << "\nReason: " << failure_reason;
        }

        if (!diagnostics.empty()) {
            oss << "\nDiagnostics:";
            for (const auto& diag : diagnostics) {
                oss << "\n  - " << diag;
            }
        }

        return oss.str();
    }

    /**
     * @brief Check if wait succeeded
     */
    operator bool() const { return success; }
};

/**
 * @brief Helper class for waiting on conditions with diagnostics
 *
 * This class provides robust waiting mechanisms for async operations
 * with proper timeout handling and diagnostic information.
 *
 * Example usage:
 * @code
 * std::atomic<bool> done{false};
 *
 * // Start async operation
 * do_async_work([&] { done = true; });
 *
 * // Wait with timeout
 * auto result = TestWaitHelper::wait_for(
 *     [&]() { return done.load(); },
 *     std::chrono::seconds(5)
 * );
 *
 * ASSERT_TRUE(result) << result.to_string();
 * @endcode
 */
class TestWaitHelper {
public:
    /**
     * @brief Wait for a condition to become true with timeout
     *
     * @tparam Predicate Callable that returns bool
     * @param pred Condition to wait for
     * @param timeout Maximum time to wait
     * @param poll_interval How often to check the condition
     * @return WaitResult with success/failure and timing info
     */
    template<typename Predicate>
    static WaitResult wait_for(
        Predicate&& pred,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000),
        std::chrono::milliseconds poll_interval = std::chrono::milliseconds(10)
    ) {
        WaitResult result;
        auto start = std::chrono::steady_clock::now();

        while (true) {
            // Check condition
            if (pred()) {
                result.success = true;
                result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start
                );
                return result;
            }

            // Check timeout
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                result.success = false;
                result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
                result.failure_reason = "Timeout after " +
                    std::to_string(result.elapsed.count()) + "ms";
                return result;
            }

            // Sleep before next check
            std::this_thread::sleep_for(poll_interval);
        }
    }

    /**
     * @brief Wait for a condition with custom diagnostic function
     *
     * @tparam Predicate Callable that returns bool
     * @tparam DiagnosticFunc Callable that returns vector<string> of diagnostics
     * @param pred Condition to wait for
     * @param diagnostic_func Function to gather diagnostics on failure
     * @param timeout Maximum time to wait
     * @param poll_interval How often to check
     * @return WaitResult with diagnostics
     */
    template<typename Predicate, typename DiagnosticFunc>
    static WaitResult wait_with_diagnostics(
        Predicate&& pred,
        DiagnosticFunc&& diagnostic_func,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000),
        std::chrono::milliseconds poll_interval = std::chrono::milliseconds(10)
    ) {
        WaitResult result = wait_for(
            std::forward<Predicate>(pred),
            timeout,
            poll_interval
        );

        if (!result.success) {
            // Gather diagnostics on failure
            try {
                result.diagnostics = diagnostic_func();
            } catch (const std::exception& e) {
                result.diagnostics.push_back(
                    std::string("Error gathering diagnostics: ") + e.what()
                );
            }
        }

        return result;
    }

    /**
     * @brief Wait for a counter to reach expected value
     *
     * @tparam CounterType Type of atomic counter
     * @param counter Atomic counter to check
     * @param expected Expected value
     * @param timeout Maximum time to wait
     * @return WaitResult
     */
    template<typename CounterType>
    static WaitResult wait_for_count(
        const std::atomic<CounterType>& counter,
        CounterType expected,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)
    ) {
        return wait_for(
            [&]() { return counter.load() >= expected; },
            timeout
        );
    }

    /**
     * @brief Wait for all futures to complete
     *
     * @tparam T Future value type
     * @param futures Vector of futures to wait for
     * @param timeout Maximum time to wait for all
     * @return WaitResult
     */
    template<typename T>
    static WaitResult wait_for_all(
        const std::vector<std::future<T>>& futures,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)
    ) {
        auto start = std::chrono::steady_clock::now();

        for (size_t i = 0; i < futures.size(); ++i) {
            auto remaining = timeout - std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );

            if (remaining <= std::chrono::milliseconds(0)) {
                WaitResult result;
                result.success = false;
                result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start
                );
                result.failure_reason = "Timeout waiting for future " +
                    std::to_string(i) + "/" + std::to_string(futures.size());
                return result;
            }

            if (futures[i].wait_for(remaining) == std::future_status::timeout) {
                WaitResult result;
                result.success = false;
                result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start
                );
                result.failure_reason = "Future " + std::to_string(i) +
                    " did not complete in time";
                return result;
            }
        }

        WaitResult result;
        result.success = true;
        result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );
        return result;
    }
};

} // namespace kcenon::testing
