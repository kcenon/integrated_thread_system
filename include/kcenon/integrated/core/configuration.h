// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file configuration.h
 * @brief Unified configuration for integrated_thread_system
 *
 * Provides centralized configuration management for all subsystems
 * (thread, logger, monitoring) with common_system integration.
 */

#pragma once

#include <cstddef>
#include <string>
#include <chrono>

namespace kcenon::integrated {

/**
 * @brief Log levels matching common_system and logger_system
 */
enum class log_level {
    trace,
    debug,
    info,
    warning,
    error,
    critical,
    fatal
};

/**
 * @brief Thread pool configuration
 */
struct thread_config {
    std::string name = "IntegratedThreadPool";
    std::size_t thread_count = 0;  // 0 = auto-detect
    std::size_t max_queue_size = 10000;
    bool enable_work_stealing = true;
    bool enable_dynamic_scaling = false;
    std::size_t min_threads = 1;
    std::size_t max_threads = 0;  // 0 = no limit
};

/**
 * @brief Logger configuration
 */
struct logger_config {
    bool enable_file_logging = true;
    bool enable_console_logging = true;
    bool async_mode = true;
    std::size_t buffer_size = 8192;
    std::string log_directory = "./logs";
    log_level min_log_level = log_level::info;
    bool enable_metrics = true;
};

/**
 * @brief Monitoring configuration
 */
struct monitoring_config {
    bool enable_monitoring = true;
    bool enable_profiling = true;
    bool enable_distributed_tracing = false;
    std::chrono::milliseconds sampling_interval{1000};
    bool enable_opentelemetry_export = false;
    double cpu_threshold = 80.0;
    double memory_threshold = 90.0;
};

/**
 * @brief Circuit breaker configuration
 */
struct circuit_breaker_config {
    bool enabled = false;
    std::size_t failure_threshold = 5;
    std::chrono::milliseconds reset_timeout{5000};
};

/**
 * @brief Unified configuration for all systems
 */
struct unified_config {
    // Subsystem configurations
    thread_config thread;
    logger_config logger;
    monitoring_config monitoring;
    circuit_breaker_config circuit_breaker;

    // Integration settings
    bool enable_auto_profiling = true;
    bool enable_metrics_aggregation = true;

    // Builder pattern methods
    unified_config& set_thread_count(std::size_t count) {
        thread.thread_count = count;
        return *this;
    }

    unified_config& set_log_level(log_level level) {
        logger.min_log_level = level;
        return *this;
    }

    unified_config& enable_tracing(bool enable = true) {
        monitoring.enable_distributed_tracing = enable;
        return *this;
    }

    unified_config& enable_circuit_breaker(bool enable = true) {
        circuit_breaker.enabled = enable;
        return *this;
    }
};

} // namespace kcenon::integrated
