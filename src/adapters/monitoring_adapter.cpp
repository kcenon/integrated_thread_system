// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/monitoring_adapter.h>

#if EXTERNAL_SYSTEMS_AVAILABLE
// Use external monitoring_system's performance monitor
#include <kcenon/monitoring/core/performance_monitor.h>
#else
// Fallback to built-in implementation
#include <mutex>
#include <unordered_map>
#include <chrono>
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
            // Create monitoring_system's performance profiler and system monitor
            profiler_ = std::make_unique<monitoring_system::performance_profiler>();
            system_monitor_ = std::make_unique<monitoring_system::system_monitor>();

            // Enable profiling by default
            profiler_->set_enabled(true);
            profiler_->set_max_samples(config_.max_samples_per_metric);

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
                std::string("Monitoring adapter initialization failed: ") + e.what()
            );
        }
    }

    common::VoidResult shutdown() {
        if (!initialized_) {
            return common::ok();
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (profiler_) {
            profiler_->clear_all_samples();
            profiler_.reset();
        }
        system_monitor_.reset();
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.clear();
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
        // For monitoring_system, record value as nanoseconds duration
        // This allows tracking both timing and counter metrics
        auto duration = std::chrono::nanoseconds(static_cast<int64_t>(value));
        if (profiler_) {
            auto result = profiler_->record_sample(name, duration, true);
            if (!result) {
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
        // For now, tags are ignored in both implementations
        // TODO: Future enhancement to support tagged metrics
        return record_metric(name, value);
    }

    common::Result<common::interfaces::metrics_snapshot> get_metrics() {
        if (!initialized_) {
            return common::Result<common::interfaces::metrics_snapshot>::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

        common::interfaces::metrics_snapshot snapshot;
        auto now = std::chrono::system_clock::now();

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (profiler_) {
            // Get all performance metrics from profiler
            auto all_metrics = profiler_->get_all_metrics();

            for (const auto& perf_metric : all_metrics) {
                // Add mean duration as primary metric
                common::interfaces::metric_value mv;
                mv.name = perf_metric.operation_name;
                mv.value = static_cast<double>(perf_metric.mean_duration.count());
                mv.timestamp = now;
                snapshot.metrics.push_back(mv);

                // Add call count
                common::interfaces::metric_value count_mv;
                count_mv.name = perf_metric.operation_name + ".call_count";
                count_mv.value = static_cast<double>(perf_metric.call_count);
                count_mv.timestamp = now;
                snapshot.metrics.push_back(count_mv);

                // Add p95 duration
                common::interfaces::metric_value p95_mv;
                p95_mv.name = perf_metric.operation_name + ".p95";
                p95_mv.value = static_cast<double>(perf_metric.p95_duration.count());
                p95_mv.timestamp = now;
                snapshot.metrics.push_back(p95_mv);
            }
        }

        // Get system metrics if available
        if (system_monitor_) {
            auto sys_metrics_result = system_monitor_->get_current_metrics();
            if (sys_metrics_result) {
                const auto& sys_metrics = sys_metrics_result.value();

                // Add CPU usage
                common::interfaces::metric_value cpu_mv;
                cpu_mv.name = "system.cpu_usage_percent";
                cpu_mv.value = sys_metrics.cpu_usage_percent;
                cpu_mv.timestamp = now;
                snapshot.metrics.push_back(cpu_mv);

                // Add memory usage
                common::interfaces::metric_value mem_mv;
                mem_mv.name = "system.memory_usage_percent";
                mem_mv.value = sys_metrics.memory_usage_percent;
                mem_mv.timestamp = now;
                snapshot.metrics.push_back(mem_mv);
            }
        }
#else
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        // Convert metrics map to vector of metric_value
        for (const auto& [name, value] : metrics_) {
            common::interfaces::metric_value mv;
            mv.name = name;
            mv.value = value;
            mv.timestamp = now;
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
        result.status = common::interfaces::health_status::healthy;
        result.message = "Monitoring adapter is operational";
        result.timestamp = std::chrono::system_clock::now();

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Get system health from system_monitor if available
        if (system_monitor_) {
            auto sys_metrics_result = system_monitor_->get_current_metrics();
            if (sys_metrics_result) {
                const auto& sys_metrics = sys_metrics_result.value();

                // Check if system resources are healthy
                if (sys_metrics.cpu_usage_percent > 90.0) {
                    result.status = common::interfaces::health_status::degraded;
                    result.message = "High CPU usage detected";
                } else if (sys_metrics.memory_usage_percent > 90.0) {
                    result.status = common::interfaces::health_status::degraded;
                    result.message = "High memory usage detected";
                }
            }
        }
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
        if (profiler_) {
            profiler_->clear_all_samples();
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

private:
    monitoring_config config_;
    bool initialized_;
    std::chrono::steady_clock::time_point start_time_;

#if EXTERNAL_SYSTEMS_AVAILABLE
    // External monitoring_system integration
    std::unique_ptr<monitoring_system::performance_profiler> profiler_;
    std::unique_ptr<monitoring_system::system_monitor> system_monitor_;
#else
    // Built-in implementation
    std::unordered_map<std::string, double> metrics_;
    mutable std::mutex metrics_mutex_;
#endif
};

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

} // namespace kcenon::integrated::adapters
