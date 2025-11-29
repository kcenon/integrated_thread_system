// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/monitoring_adapter.h>

#include <format>

#if EXTERNAL_SYSTEMS_AVAILABLE
// Core monitoring components
#include <kcenon/monitoring/core/performance_monitor.h>

// Adaptive monitoring (monitoring_system v4.0.0)
#include <kcenon/monitoring/adaptive/adaptive_monitor.h>

// Health monitoring (monitoring_system v4.0.0)
#include <kcenon/monitoring/health/health_monitor.h>

// Circuit breaker (monitoring_system v4.0.0)
#include <kcenon/monitoring/reliability/circuit_breaker.h>
#else
// Fallback to built-in implementation
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <vector>
#endif

namespace kcenon::integrated::adapters {

class monitoring_adapter::impl {
public:
    explicit impl(const monitoring_config& config)
        : config_(config)
        , initialized_(false) {
    }

    ~impl() {
        if (initialized_) {
            shutdown();
        }
    }

    common::VoidResult initialize() {
        if (initialized_) {
            return common::ok();
        }

        try {
#if EXTERNAL_SYSTEMS_AVAILABLE
            // Initialize performance monitor for metrics collection
            performance_monitor_ = std::make_unique<kcenon::monitoring::performance_monitor>(
                "integrated_thread_system"
            );
            auto init_result = performance_monitor_->initialize();
            if (!init_result.is_ok()) {
                return common::VoidResult::err(
                    common::error_codes::INTERNAL_ERROR,
                    "Failed to initialize performance monitor"
                );
            }

            // Configure thresholds from config
            performance_monitor_->set_cpu_threshold(config_.cpu_threshold);
            performance_monitor_->set_memory_threshold(config_.memory_threshold);

            // Start system monitoring
            auto& sys_monitor = performance_monitor_->get_system_monitor();
            sys_monitor.start_monitoring(config_.sampling_interval);

            // Initialize adaptive monitoring
            if (config_.enable_adaptive_monitoring) {
                adaptive_monitor_ = std::make_unique<kcenon::monitoring::adaptive_monitor>();

                // Configure adaptive settings
                kcenon::monitoring::adaptive_config adaptive_cfg;
                adaptive_cfg.low_threshold = config_.adaptive_low_threshold * 100.0;
                adaptive_cfg.moderate_threshold = (config_.adaptive_low_threshold + config_.adaptive_high_threshold) / 2.0 * 100.0;
                adaptive_cfg.high_threshold = config_.adaptive_high_threshold * 100.0;
                adaptive_cfg.low_interval = config_.adaptive_min_interval;
                adaptive_cfg.high_interval = config_.adaptive_max_interval;

                // Register performance monitor as a collector
                adaptive_monitor_->register_collector(
                    "performance",
                    std::shared_ptr<kcenon::monitoring::performance_monitor>(
                        performance_monitor_.get(),
                        [](kcenon::monitoring::performance_monitor*) {} // No-op deleter
                    ),
                    adaptive_cfg
                );

                auto start_result = adaptive_monitor_->start();
                if (!start_result.is_ok()) {
                    return common::VoidResult::err(
                        common::error_codes::INTERNAL_ERROR,
                        "Failed to start adaptive monitor"
                    );
                }
            }

            // Initialize health monitoring
            if (config_.enable_health_monitoring) {
                kcenon::monitoring::health_monitor_config health_cfg;
                health_cfg.check_interval = config_.health_check_interval;
                health_cfg.enable_auto_recovery = true;
                health_monitor_ = std::make_unique<kcenon::monitoring::health_monitor>(health_cfg);
                health_monitor_->start();
            }

            initialized_ = true;
            start_time_ = std::chrono::steady_clock::now();
            return common::ok();
#else
            // Built-in in-memory metrics storage
            initialized_ = true;
            start_time_ = std::chrono::steady_clock::now();
            return common::ok();
#endif
        } catch (const std::exception& e) {
            return common::VoidResult::err(
                common::error_codes::INTERNAL_ERROR,
                std::format("Monitoring adapter initialization failed: {}", e.what())
            );
        }
    }

    common::VoidResult shutdown() {
        if (!initialized_) {
            return common::ok();
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Stop adaptive monitoring
        if (adaptive_monitor_) {
            adaptive_monitor_->stop();
            adaptive_monitor_.reset();
        }

        // Stop health monitoring
        if (health_monitor_) {
            health_monitor_->stop();
            health_monitor_.reset();
        }

        // Clean up performance monitor
        if (performance_monitor_) {
            performance_monitor_->get_system_monitor().stop_monitoring();
            performance_monitor_->cleanup();
            performance_monitor_.reset();
        }
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.clear();
        health_checks_.clear();
#endif

        initialized_ = false;
        return common::ok();
    }

    bool is_initialized() const {
        return initialized_;
    }

    common::VoidResult record_metric(const std::string& name, double value) {
        if (!initialized_) {
            return common::VoidResult::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (performance_monitor_) {
            // Record as a duration metric (treating value as nanoseconds for timing metrics)
            auto duration = std::chrono::nanoseconds(static_cast<int64_t>(value));
            auto result = performance_monitor_->get_profiler().record_sample(name, duration, true);
            if (!result.is_ok()) {
                return common::VoidResult::err(
                    common::error_codes::INTERNAL_ERROR,
                    "Failed to record metric"
                );
            }
        }
        return common::ok();
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_[name] = value;
        return common::ok();
#endif
    }

    common::VoidResult record_metric(const std::string& name, double value,
                                     const std::unordered_map<std::string, std::string>& tags) {
        // Record base metric
        auto result = record_metric(name, value);
        if (!result.is_ok()) {
            return result;
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Tags are stored in metrics snapshot when collecting
        // For now, we append tags to the metric name for differentiation
        if (!tags.empty()) {
            std::string tagged_name = name;
            for (const auto& [key, val] : tags) {
                tagged_name += std::format(".{}={}", key, val);
            }
            return record_metric(tagged_name, value);
        }
#endif
        return common::ok();
    }

    common::Result<common::interfaces::metrics_snapshot> get_metrics() {
        if (!initialized_) {
            return common::Result<common::interfaces::metrics_snapshot>::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

        common::interfaces::metrics_snapshot snapshot;
        snapshot.capture_time = std::chrono::system_clock::now();
        snapshot.source_id = "integrated_thread_system";

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Get performance metrics from profiler
        if (performance_monitor_) {
            auto all_metrics = performance_monitor_->get_profiler().get_all_metrics();
            for (const auto& pm : all_metrics) {
                common::interfaces::metric_value mv;
                mv.name = pm.operation_name + ".mean_ns";
                mv.value = static_cast<double>(pm.mean_duration.count());
                mv.type = common::interfaces::metric_type::gauge;
                mv.timestamp = snapshot.capture_time;
                snapshot.metrics.push_back(mv);

                mv.name = pm.operation_name + ".p95_ns";
                mv.value = static_cast<double>(pm.p95_duration.count());
                snapshot.metrics.push_back(mv);

                mv.name = pm.operation_name + ".p99_ns";
                mv.value = static_cast<double>(pm.p99_duration.count());
                snapshot.metrics.push_back(mv);

                mv.name = pm.operation_name + ".call_count";
                mv.value = static_cast<double>(pm.call_count);
                mv.type = common::interfaces::metric_type::counter;
                snapshot.metrics.push_back(mv);

                mv.name = pm.operation_name + ".error_count";
                mv.value = static_cast<double>(pm.error_count);
                snapshot.metrics.push_back(mv);

                mv.name = pm.operation_name + ".throughput";
                mv.value = pm.throughput;
                mv.type = common::interfaces::metric_type::gauge;
                snapshot.metrics.push_back(mv);
            }
        }

        // Get system metrics
        if (performance_monitor_) {
            auto sys_result = performance_monitor_->get_system_monitor().get_current_metrics();
            if (sys_result.is_ok()) {
                const auto& sys = sys_result.value();
                snapshot.add_metric("system.cpu_usage_percent", sys.cpu_usage_percent);
                snapshot.add_metric("system.memory_usage_percent", sys.memory_usage_percent);
                snapshot.add_metric("system.memory_usage_bytes", static_cast<double>(sys.memory_usage_bytes));
                snapshot.add_metric("system.thread_count", static_cast<double>(sys.thread_count));
                snapshot.add_metric("system.disk_io_read_rate", sys.disk_io_read_rate);
                snapshot.add_metric("system.disk_io_write_rate", sys.disk_io_write_rate);
                snapshot.add_metric("system.network_recv_rate", sys.network_io_recv_rate);
                snapshot.add_metric("system.network_send_rate", sys.network_io_send_rate);
            }
        }

        // Get adaptation stats if available
        if (adaptive_monitor_) {
            auto all_stats = adaptive_monitor_->get_all_stats();
            for (const auto& [name, stats] : all_stats) {
                snapshot.add_metric(std::format("adaptive.{}.sampling_rate", name), stats.current_sampling_rate);
                snapshot.add_metric(std::format("adaptive.{}.samples_collected", name),
                                    static_cast<double>(stats.samples_collected));
                snapshot.add_metric(std::format("adaptive.{}.samples_dropped", name),
                                    static_cast<double>(stats.samples_dropped));
            }
        }
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        // Convert metrics map to vector of metric_value
        for (const auto& [name, value] : metrics_) {
            common::interfaces::metric_value mv;
            mv.name = name;
            mv.value = value;
            mv.timestamp = snapshot.capture_time;
            snapshot.metrics.push_back(mv);
        }
#endif

        return common::Result<common::interfaces::metrics_snapshot>::ok(snapshot);
    }

    common::Result<common::interfaces::health_check_result> check_health() {
        if (!initialized_) {
            return common::Result<common::interfaces::health_check_result>::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

        common::interfaces::health_check_result result;
        result.timestamp = std::chrono::system_clock::now();

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (health_monitor_) {
            auto check_start = std::chrono::steady_clock::now();
            auto health_result = health_monitor_->check_health();
            auto check_end = std::chrono::steady_clock::now();

            result.check_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                check_end - check_start
            );

            // Map monitoring_system health_status to common health_status
            switch (health_result.status) {
                case kcenon::monitoring::health_status::healthy:
                    result.status = common::interfaces::health_status::healthy;
                    break;
                case kcenon::monitoring::health_status::degraded:
                    result.status = common::interfaces::health_status::degraded;
                    break;
                case kcenon::monitoring::health_status::unhealthy:
                    result.status = common::interfaces::health_status::unhealthy;
                    break;
                default:
                    result.status = common::interfaces::health_status::unknown;
                    break;
            }

            result.message = health_result.message;

            // Check all registered health checks
            auto all_results = health_monitor_->check_all();
            std::vector<std::string> failed_checks;
            for (const auto& [name, check_result] : all_results) {
                result.metadata[name] = (check_result.status == kcenon::monitoring::health_status::healthy)
                    ? "healthy" : "unhealthy";
                if (check_result.status != kcenon::monitoring::health_status::healthy) {
                    failed_checks.push_back(name);
                }
            }

            if (!failed_checks.empty()) {
                result.metadata["failed_checks"] = std::to_string(failed_checks.size());
            }
        } else {
            result.status = common::interfaces::health_status::healthy;
            result.message = "Monitoring adapter is operational (health monitoring disabled)";
        }

        // Check threshold violations
        if (performance_monitor_) {
            auto threshold_result = performance_monitor_->check_thresholds();
            if (threshold_result.is_ok() && !threshold_result.value()) {
                if (result.status == common::interfaces::health_status::healthy) {
                    result.status = common::interfaces::health_status::degraded;
                }
                result.metadata["threshold_violation"] = "true";
            }
        }
#else
        auto check_start = std::chrono::steady_clock::now();

        // Run fallback health checks
        bool all_healthy = true;
        std::vector<std::string> failed_checks;

        {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            for (const auto& [name, check] : health_checks_) {
                try {
                    if (!check()) {
                        all_healthy = false;
                        failed_checks.push_back(name);
                        result.metadata[name] = "unhealthy";
                    } else {
                        result.metadata[name] = "healthy";
                    }
                } catch (...) {
                    all_healthy = false;
                    failed_checks.push_back(name);
                    result.metadata[name] = "error";
                }
            }
        }

        auto check_end = std::chrono::steady_clock::now();
        result.check_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            check_end - check_start
        );

        result.status = all_healthy ? common::interfaces::health_status::healthy
                                    : common::interfaces::health_status::degraded;
        result.message = all_healthy ? "All health checks passed"
                                     : std::format("{} health check(s) failed", failed_checks.size());
#endif

        return common::Result<common::interfaces::health_check_result>::ok(result);
    }

    common::VoidResult reset() {
        if (!initialized_) {
            return common::VoidResult::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (performance_monitor_) {
            performance_monitor_->get_profiler().clear_all_samples();
            performance_monitor_->reset();
        }
        start_time_ = std::chrono::steady_clock::now();
        return common::ok();
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.clear();
        start_time_ = std::chrono::steady_clock::now();
        return common::ok();
#endif
    }

    common::VoidResult register_health_check(const std::string& name, health_check_func check) {
        if (!initialized_) {
            return common::VoidResult::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (health_monitor_) {
            // Wrap the simple bool function into a functional_health_check
            auto functional_check = std::make_shared<kcenon::monitoring::functional_health_check>(
                name,
                kcenon::monitoring::health_check_type::liveness,
                [check]() -> kcenon::monitoring::health_check_result {
                    kcenon::monitoring::health_check_result result;
                    result.timestamp = std::chrono::system_clock::now();
                    try {
                        if (check()) {
                            result.status = kcenon::monitoring::health_status::healthy;
                            result.message = "Check passed";
                        } else {
                            result.status = kcenon::monitoring::health_status::unhealthy;
                            result.message = "Check failed";
                        }
                    } catch (const std::exception& e) {
                        result.status = kcenon::monitoring::health_status::unhealthy;
                        result.message = std::format("Check threw exception: {}", e.what());
                    }
                    return result;
                },
                std::chrono::milliseconds(1000),
                false
            );
            health_monitor_->register_check(name, functional_check);
        }
        return common::ok();
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        health_checks_[name] = check;
        return common::ok();
#endif
    }

    common::VoidResult unregister_health_check(const std::string& name) {
        if (!initialized_) {
            return common::VoidResult::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Note: health_monitor may not support unregistration in current API
        // This is a placeholder for future API enhancement
        return common::ok();
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        health_checks_.erase(name);
        return common::ok();
#endif
    }

    common::Result<adaptation_stats> get_adaptation_stats() const {
        if (!initialized_) {
            return common::Result<adaptation_stats>::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

        adaptation_stats stats;

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (adaptive_monitor_) {
            auto all_stats = adaptive_monitor_->get_all_stats();
            // Aggregate stats from all collectors
            for (const auto& [name, collector_stats] : all_stats) {
                stats.total_adaptations += collector_stats.total_adaptations;
                stats.upscale_count += collector_stats.upscale_count;
                stats.downscale_count += collector_stats.downscale_count;
                stats.samples_dropped += collector_stats.samples_dropped;
                stats.samples_collected += collector_stats.samples_collected;
                stats.average_cpu_usage = collector_stats.average_cpu_usage;
                stats.average_memory_usage = collector_stats.average_memory_usage;
                stats.current_sampling_rate = collector_stats.current_sampling_rate;
                stats.current_interval = collector_stats.current_interval;
            }
        }
#endif

        return common::Result<adaptation_stats>::ok(stats);
    }

    common::Result<system_resource_metrics> get_system_resources() const {
        if (!initialized_) {
            return common::Result<system_resource_metrics>::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

        system_resource_metrics resources;
        resources.timestamp = std::chrono::system_clock::now();

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (performance_monitor_) {
            auto sys_result = performance_monitor_->get_system_monitor().get_current_metrics();
            if (sys_result.is_ok()) {
                const auto& sys = sys_result.value();
                resources.cpu_usage_percent = sys.cpu_usage_percent;
                resources.memory_usage_percent = sys.memory_usage_percent;
                resources.memory_usage_bytes = sys.memory_usage_bytes;
                resources.available_memory_bytes = sys.available_memory_bytes;
                resources.disk_read_bytes_per_sec = static_cast<std::size_t>(sys.disk_io_read_rate);
                resources.disk_write_bytes_per_sec = static_cast<std::size_t>(sys.disk_io_write_rate);
                resources.network_rx_bytes_per_sec = static_cast<std::size_t>(sys.network_io_recv_rate);
                resources.network_tx_bytes_per_sec = static_cast<std::size_t>(sys.network_io_send_rate);
                resources.thread_count = sys.thread_count;
            }
        }

#endif

        return common::Result<system_resource_metrics>::ok(resources);
    }

    common::Result<circuit_breaker_metrics> get_circuit_breaker_metrics(const std::string& name) const {
        if (!initialized_) {
            return common::Result<circuit_breaker_metrics>::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

        circuit_breaker_metrics metrics;

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Circuit breaker metrics are typically managed per-operation
        // This provides a way to query circuit breaker state if one exists
        auto it = circuit_breakers_.find(name);
        if (it != circuit_breakers_.end()) {
            auto cb_metrics = it->second->get_metrics();
            metrics.total_calls = cb_metrics.total_calls.load();
            metrics.successful_calls = cb_metrics.successful_calls.load();
            metrics.failed_calls = cb_metrics.failed_calls.load();
            metrics.rejected_calls = cb_metrics.rejected_calls.load();
            metrics.state_transitions = cb_metrics.state_transitions.load();

            auto state = it->second->get_state();
            switch (state) {
                case kcenon::monitoring::circuit_state::closed:
                    metrics.current_state = "closed";
                    break;
                case kcenon::monitoring::circuit_state::open:
                    metrics.current_state = "open";
                    break;
                case kcenon::monitoring::circuit_state::half_open:
                    metrics.current_state = "half_open";
                    break;
            }
        } else {
            return common::Result<circuit_breaker_metrics>::err(
                common::error_codes::NOT_FOUND,
                std::format("Circuit breaker '{}' not found", name)
            );
        }
#else
        return common::Result<circuit_breaker_metrics>::err(
            common::error_codes::NOT_SUPPORTED,
            "Circuit breaker monitoring requires external monitoring_system"
        );
#endif

        return common::Result<circuit_breaker_metrics>::ok(metrics);
    }

    void record_operation_timing(const std::string& name, std::chrono::nanoseconds duration, bool success) {
#if EXTERNAL_SYSTEMS_AVAILABLE
        if (performance_monitor_) {
            performance_monitor_->get_profiler().record_sample(name, duration, success);
        }
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_[name + ".last_duration_ns"] = static_cast<double>(duration.count());
        if (!success) {
            metrics_[name + ".error_count"] = metrics_[name + ".error_count"] + 1;
        }
#endif
    }

private:
    monitoring_config config_;
    bool initialized_;
    std::chrono::steady_clock::time_point start_time_;

    using health_check_func = std::function<bool()>;

#if EXTERNAL_SYSTEMS_AVAILABLE
    // External monitoring_system integration
    std::unique_ptr<kcenon::monitoring::performance_monitor> performance_monitor_;
    std::unique_ptr<kcenon::monitoring::adaptive_monitor> adaptive_monitor_;
    std::unique_ptr<kcenon::monitoring::health_monitor> health_monitor_;
    std::unordered_map<std::string, std::shared_ptr<kcenon::monitoring::circuit_breaker<>>> circuit_breakers_;
#else
    // Built-in implementation
    std::unordered_map<std::string, double> metrics_;
    std::unordered_map<std::string, health_check_func> health_checks_;
    mutable std::mutex metrics_mutex_;
#endif
};

// scoped_timer implementation

monitoring_adapter::scoped_timer::scoped_timer(monitoring_adapter* adapter, const std::string& operation_name)
    : adapter_(adapter)
    , operation_name_(operation_name)
    , start_(std::chrono::steady_clock::now())
    , failed_(false) {
}

monitoring_adapter::scoped_timer::~scoped_timer() {
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_);
    if (adapter_ && adapter_->pimpl_) {
        adapter_->pimpl_->record_operation_timing(operation_name_, duration, !failed_);
    }
}

void monitoring_adapter::scoped_timer::mark_failed() {
    failed_ = true;
}

// monitoring_adapter implementation

monitoring_adapter::monitoring_adapter(const monitoring_config& config)
    : pimpl_(std::make_unique<impl>(config)) {
}

monitoring_adapter::~monitoring_adapter() = default;

monitoring_adapter::monitoring_adapter(monitoring_adapter&&) noexcept = default;
monitoring_adapter& monitoring_adapter::operator=(monitoring_adapter&&) noexcept = default;

common::VoidResult monitoring_adapter::initialize() {
    return pimpl_->initialize();
}

common::VoidResult monitoring_adapter::shutdown() {
    return pimpl_->shutdown();
}

bool monitoring_adapter::is_initialized() const {
    return pimpl_->is_initialized();
}

common::VoidResult monitoring_adapter::record_metric(const std::string& name, double value) {
    return pimpl_->record_metric(name, value);
}

common::VoidResult monitoring_adapter::record_metric(const std::string& name, double value,
                                                     const std::unordered_map<std::string, std::string>& tags) {
    return pimpl_->record_metric(name, value, tags);
}

common::Result<common::interfaces::metrics_snapshot> monitoring_adapter::get_metrics() {
    return pimpl_->get_metrics();
}

common::Result<common::interfaces::health_check_result> monitoring_adapter::check_health() {
    return pimpl_->check_health();
}

common::VoidResult monitoring_adapter::reset() {
    return pimpl_->reset();
}

common::VoidResult monitoring_adapter::register_health_check(const std::string& name, health_check_func check) {
    return pimpl_->register_health_check(name, check);
}

common::VoidResult monitoring_adapter::unregister_health_check(const std::string& name) {
    return pimpl_->unregister_health_check(name);
}

common::Result<adaptation_stats> monitoring_adapter::get_adaptation_stats() const {
    return pimpl_->get_adaptation_stats();
}

common::Result<system_resource_metrics> monitoring_adapter::get_system_resources() const {
    return pimpl_->get_system_resources();
}

common::Result<circuit_breaker_metrics> monitoring_adapter::get_circuit_breaker_metrics(const std::string& name) const {
    return pimpl_->get_circuit_breaker_metrics(name);
}

monitoring_adapter::scoped_timer monitoring_adapter::time_operation(const std::string& operation_name) {
    return scoped_timer(this, operation_name);
}

} // namespace kcenon::integrated::adapters
