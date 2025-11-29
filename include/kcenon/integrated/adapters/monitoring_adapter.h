// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file monitoring_adapter.h
 * @brief Adapter for monitoring_system integration
 *
 * Integrates monitoring_system v4.0.0 features:
 * - adaptive_monitor: Load-based sampling adjustment
 * - health_monitor: Component health checks
 * - performance_monitor: Performance profiling
 * - system_resource_collector: System metrics collection
 * - circuit_breaker monitoring: Failure pattern tracking
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/interfaces/monitoring_interface.h>
#include <kcenon/integrated/core/configuration.h>

namespace kcenon::integrated::adapters {

/**
 * @brief Adaptation statistics from adaptive monitoring
 */
struct adaptation_stats {
    std::uint64_t total_adaptations{0};
    std::uint64_t upscale_count{0};
    std::uint64_t downscale_count{0};
    std::uint64_t samples_dropped{0};
    std::uint64_t samples_collected{0};
    double average_cpu_usage{0.0};
    double average_memory_usage{0.0};
    double current_sampling_rate{1.0};
    std::chrono::milliseconds current_interval{1000};
};

/**
 * @brief System resource metrics
 */
struct system_resource_metrics {
    double cpu_usage_percent{0.0};
    double memory_usage_percent{0.0};
    std::size_t memory_usage_bytes{0};
    std::size_t available_memory_bytes{0};
    std::size_t disk_read_bytes_per_sec{0};
    std::size_t disk_write_bytes_per_sec{0};
    std::size_t network_rx_bytes_per_sec{0};
    std::size_t network_tx_bytes_per_sec{0};
    std::uint32_t thread_count{0};
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Circuit breaker metrics
 */
struct circuit_breaker_metrics {
    std::size_t total_calls{0};
    std::size_t successful_calls{0};
    std::size_t failed_calls{0};
    std::size_t rejected_calls{0};
    std::size_t state_transitions{0};
    std::string current_state;  // "closed", "open", "half_open"
};

/**
 * @brief Adapter for monitoring_system integration
 *
 * Provides comprehensive monitoring capabilities by integrating
 * monitoring_system v4.0.0 features including adaptive monitoring,
 * health checks, performance profiling, and system resource collection.
 */
class monitoring_adapter : public common::interfaces::IMonitor {
public:
    explicit monitoring_adapter(const monitoring_config& config);
    ~monitoring_adapter() override;

    monitoring_adapter(const monitoring_adapter&) = delete;
    monitoring_adapter& operator=(const monitoring_adapter&) = delete;
    monitoring_adapter(monitoring_adapter&&) noexcept;
    monitoring_adapter& operator=(monitoring_adapter&&) noexcept;

    // Lifecycle management
    common::VoidResult initialize();
    common::VoidResult shutdown();
    bool is_initialized() const;

    // IMonitor interface implementation
    common::VoidResult record_metric(const std::string& name, double value) override;
    common::VoidResult record_metric(const std::string& name, double value,
                                     const std::unordered_map<std::string, std::string>& tags) override;
    common::Result<common::interfaces::metrics_snapshot> get_metrics() override;
    common::Result<common::interfaces::health_check_result> check_health() override;
    common::VoidResult reset() override;

    // Health check registration (monitoring_system v4.0.0)
    using health_check_func = std::function<bool()>;
    common::VoidResult register_health_check(const std::string& name, health_check_func check);
    common::VoidResult unregister_health_check(const std::string& name);

    // Adaptation statistics (monitoring_system v4.0.0)
    common::Result<adaptation_stats> get_adaptation_stats() const;

    // System resource metrics (monitoring_system v4.0.0)
    common::Result<system_resource_metrics> get_system_resources() const;

    // Circuit breaker metrics (monitoring_system v4.0.0)
    common::Result<circuit_breaker_metrics> get_circuit_breaker_metrics(const std::string& name) const;

    // Performance timing helper
    class scoped_timer {
    public:
        scoped_timer(monitoring_adapter* adapter, const std::string& operation_name);
        ~scoped_timer();
        void mark_failed();
    private:
        monitoring_adapter* adapter_;
        std::string operation_name_;
        std::chrono::steady_clock::time_point start_;
        bool failed_{false};
    };

    scoped_timer time_operation(const std::string& operation_name);

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace kcenon::integrated::adapters
