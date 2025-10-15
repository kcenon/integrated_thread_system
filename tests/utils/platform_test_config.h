// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file platform_test_config.h
 * @brief Platform-specific test configuration and utilities
 *
 * Phase 2.1: Platform-aware test configuration
 */

#pragma once

#include <chrono>
#include <cstdlib>
#include <string>

namespace kcenon::testing {

/**
 * @brief Platform detection and configuration for tests
 *
 * Provides platform-specific timeouts, strategies, and feature detection
 * to handle differences between Windows, macOS, and Linux.
 */
class PlatformTestConfig {
public:
    /**
     * @brief Check if running on Windows
     */
    static constexpr bool is_windows() {
#ifdef _WIN32
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Check if running on macOS
     */
    static constexpr bool is_macos() {
#ifdef __APPLE__
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Check if running on Linux
     */
    static constexpr bool is_linux() {
#ifdef __linux__
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Check if running in CI environment
     */
    static bool is_ci() {
        // Common CI environment variables
        return std::getenv("CI") != nullptr ||
               std::getenv("CONTINUOUS_INTEGRATION") != nullptr ||
               std::getenv("GITHUB_ACTIONS") != nullptr ||
               std::getenv("GITLAB_CI") != nullptr;
    }

    /**
     * @brief Get platform name as string
     */
    static const char* platform_name() {
        if (is_windows()) return "Windows";
        if (is_macos()) return "macOS";
        if (is_linux()) return "Linux";
        return "Unknown";
    }

    /**
     * @brief Get default timeout for async event delivery
     *
     * Windows may need longer timeouts due to scheduling differences
     */
    static std::chrono::milliseconds event_delivery_timeout() {
        if (is_ci()) {
            // CI environments are often slower
            return is_windows()
                ? std::chrono::milliseconds(10000)  // 10s on Windows CI
                : std::chrono::milliseconds(5000);   // 5s on Unix CI
        }

        return is_windows()
            ? std::chrono::milliseconds(5000)   // 5s on Windows
            : std::chrono::milliseconds(1000);  // 1s on Unix
    }

    /**
     * @brief Get poll interval for condition checking
     *
     * More frequent polling on Unix, less frequent on Windows
     */
    static std::chrono::milliseconds poll_interval() {
        return is_windows()
            ? std::chrono::milliseconds(50)   // 50ms on Windows
            : std::chrono::milliseconds(10);  // 10ms on Unix
    }

    /**
     * @brief Get timeout for system initialization
     */
    static std::chrono::milliseconds init_timeout() {
        if (is_ci()) {
            return std::chrono::milliseconds(5000);  // 5s in CI
        }
        return std::chrono::milliseconds(2000);  // 2s locally
    }

    /**
     * @brief Get timeout for system shutdown
     */
    static std::chrono::milliseconds shutdown_timeout() {
        return std::chrono::milliseconds(2000);  // 2s everywhere
    }

    /**
     * @brief Get expected task throughput (tasks/sec)
     *
     * Different platforms have different performance characteristics
     */
    static double expected_task_throughput() {
        // These are rough estimates for single-threaded task processing
        if (is_windows()) return 10000.0;   // 10k tasks/sec
        if (is_macos()) return 50000.0;     // 50k tasks/sec (typically faster)
        if (is_linux()) return 30000.0;     // 30k tasks/sec
        return 10000.0;
    }

    /**
     * @brief Check if async EventBus delivery is supported
     *
     * Can be overridden by environment variable for debugging
     */
    static bool async_eventbus_supported() {
        const char* skip = std::getenv("SKIP_ASYNC_EVENTBUS_TESTS");
        if (skip && std::string(skip) == "1") {
            return false;
        }

        // Known issue: EventBus async delivery broken on Windows
        if (is_windows()) {
            const char* force = std::getenv("FORCE_ASYNC_EVENTBUS_TESTS");
            return force && std::string(force) == "1";
        }

        return true;  // Works on Unix
    }

    /**
     * @brief Check if running with ThreadSanitizer
     */
    static bool is_tsan_enabled() {
#if defined(__SANITIZE_THREAD__)
        return true;
#elif defined(__has_feature)
  #if __has_feature(thread_sanitizer)
        return true;
  #endif
#endif
        return false;
    }

    /**
     * @brief Check if running with AddressSanitizer
     */
    static bool is_asan_enabled() {
#if defined(__SANITIZE_ADDRESS__)
        return true;
#elif defined(__has_feature)
  #if __has_feature(address_sanitizer)
        return true;
  #endif
#endif
        return false;
    }

    /**
     * @brief Get timeout multiplier for sanitizer builds
     *
     * Sanitizers significantly slow down execution
     */
    static int sanitizer_timeout_multiplier() {
        if (is_tsan_enabled()) return 10;  // TSan is ~10x slower
        if (is_asan_enabled()) return 3;   // ASan is ~3x slower
        return 1;
    }

    /**
     * @brief Get adjusted timeout accounting for sanitizers
     */
    static std::chrono::milliseconds adjusted_timeout(
        std::chrono::milliseconds base_timeout
    ) {
        return base_timeout * sanitizer_timeout_multiplier();
    }

    /**
     * @brief Get number of hardware threads
     */
    static unsigned int hardware_concurrency() {
        unsigned int threads = std::thread::hardware_concurrency();
        return threads > 0 ? threads : 4;  // Default to 4 if unknown
    }

    /**
     * @brief Get recommended thread count for tests
     *
     * Tests should use fewer threads than available to avoid oversubscription
     */
    static unsigned int test_thread_count() {
        unsigned int hw_threads = hardware_concurrency();
        // Use half of available threads, minimum 2, maximum 8
        unsigned int test_threads = hw_threads / 2;
        if (test_threads < 2) test_threads = 2;
        if (test_threads > 8) test_threads = 8;
        return test_threads;
    }

    /**
     * @brief Check if high-volume tests should be run
     *
     * High-volume tests may be skipped in CI to save time
     */
    static bool run_high_volume_tests() {
        if (is_ci()) {
            const char* run = std::getenv("RUN_HIGH_VOLUME_TESTS");
            return run && std::string(run) == "1";
        }
        return true;  // Run locally by default
    }

    /**
     * @brief Get event count for high-volume tests
     */
    static int high_volume_event_count() {
        if (is_ci()) {
            return 100;  // Reduced count in CI
        }
        return 1000;  // Full count locally
    }

    /**
     * @brief Print platform configuration info
     */
    static std::string configuration_info() {
        std::ostringstream oss;
        oss << "Platform Test Configuration:\n"
            << "  Platform: " << platform_name() << "\n"
            << "  CI: " << (is_ci() ? "Yes" : "No") << "\n"
            << "  Hardware threads: " << hardware_concurrency() << "\n"
            << "  Test threads: " << test_thread_count() << "\n"
            << "  Event delivery timeout: " << event_delivery_timeout().count() << "ms\n"
            << "  Poll interval: " << poll_interval().count() << "ms\n"
            << "  Async EventBus: " << (async_eventbus_supported() ? "Supported" : "Not supported") << "\n"
            << "  TSan: " << (is_tsan_enabled() ? "Enabled" : "Disabled") << "\n"
            << "  ASan: " << (is_asan_enabled() ? "Enabled" : "Disabled");
        return oss.str();
    }
};

} // namespace kcenon::testing
