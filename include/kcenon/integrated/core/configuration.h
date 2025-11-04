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
 * @brief Thread pool type enumeration
 */
enum class thread_pool_type {
    standard,      // Use standard thread_pool
    typed,         // Use typed_thread_pool with priority support
};

/**
 * @brief Thread pool configuration
 */
struct thread_config {
    std::string name = "IntegratedThreadPool";
    std::string pool_name = "integrated_pool";  // Internal pool name
    thread_pool_type pool_type = thread_pool_type::standard;
    std::size_t thread_count = 0;  // 0 = auto-detect
    std::size_t max_queue_size = 10000;
    bool enable_work_stealing = true;
    bool enable_dynamic_scaling = false;
    std::size_t min_threads = 1;
    std::size_t max_threads = 0;  // 0 = no limit
    bool enable_priority_scheduling = false;  // Enable for typed_thread_pool
};

/**
 * @brief Log format enumeration
 */
enum class log_format {
    timestamp,  // Human-readable timestamp format (default)
    json,       // JSON structured format for log aggregation systems
    custom      // User-defined custom formatter
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

    // Formatter options (logger_system v3.0.0+)
    log_format format = log_format::timestamp;
    bool pretty_print_json = false;  // For JSON format only
    bool include_thread_id = true;
    bool include_source_location = true;
    bool enable_colors = true;  // ANSI color codes for timestamp format
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
    std::size_t max_samples_per_metric = 10000;  // Maximum samples to store per metric
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
