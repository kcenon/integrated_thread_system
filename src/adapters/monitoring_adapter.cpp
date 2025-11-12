// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/monitoring_adapter.h>

#if EXTERNAL_SYSTEMS_AVAILABLE
// Use external monitoring_system's performance monitor
#include <kcenon/monitoring/core/performance_monitor.h>

// New adapters and features (monitoring_system v2.0.0+) - Commented out until API stabilizes
// #include <kcenon/monitoring/adapters/common_monitor_adapter.h>
// #include <kcenon/monitoring/adaptive/adaptive_monitor.h>
// #include <kcenon/monitoring/health/health_monitor.h>
// #include <kcenon/monitoring/collectors/thread_system_collector.h>
// #include <kcenon/monitoring/collectors/logger_system_collector.h>
// #include <kcenon/monitoring/collectors/system_resource_collector.h>
// #include <kcenon/monitoring/collectors/plugin_metric_collector.h>
// #include <kcenon/monitoring/reliability/circuit_breaker.h>
// #include <kcenon/monitoring/reliability/error_boundary.h>
// #include <kcenon/monitoring/reliability/fault_tolerance_manager.h>
// #include <kcenon/monitoring/reliability/retry_policy.h>
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
            // TODO: Initialize monitoring_system components when linking is stable
            // profiler_ = std::make_unique<kcenon::monitoring::performance_profiler>();
            // system_monitor_ = std::make_unique<kcenon::monitoring::system_monitor>();

            // TODO: Initialize new v2.0 features when APIs are stable
            // - common_monitor_adapter for standard interface
            // - Adaptive monitoring for efficient sampling
            // - Health monitoring capabilities
            // - Collectors (thread, logger, system resource)
            // - Reliability features (circuit breaker, error boundary, fault tolerance)

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
        // TODO: Clear resources when monitoring_system is stable
        // if (profiler_) {
        //     profiler_->clear_all_samples();
        //     profiler_.reset();
        // }
        // system_monitor_.reset();
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
        // TODO: Record metrics when monitoring_system linking is stable
        // if (profiler_) {
        //     auto duration = std::chrono::nanoseconds(static_cast<int64_t>(value));
        //     auto result = profiler_->record_sample(name, duration, true);
        //     ...
        // }
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
        // TODO: Get metrics from profiler when API is stable
        // if (profiler_) {
        //     auto all_metrics = profiler_->get_all_metrics();
        //     ...
        // }

        // TODO: Get system metrics when API is stable
        // if (system_monitor_) {
        //     auto sys_metrics_result = system_monitor_->get_current_metrics();
        //     ...
        // }
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
        // TODO: Get system health when API is stable
        // if (system_monitor_) {
        //     auto sys_metrics_result = system_monitor_->get_current_metrics();
        //     ...
        // }
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
        // TODO: Clear samples when monitoring_system is stable
        // if (profiler_) {
        //     profiler_->clear_all_samples();
        // }
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
    // External monitoring_system integration - commented out until linking is stable
    // std::unique_ptr<kcenon::monitoring::performance_profiler> profiler_;
    // std::unique_ptr<kcenon::monitoring::system_monitor> system_monitor_;

    // TODO: Add new adapters and features when APIs are stable
    // std::unique_ptr<kcenon::monitoring::adapters::common_monitor_adapter> monitor_adapter_;
    // std::shared_ptr<kcenon::monitoring::adaptive::adaptive_monitor> adaptive_monitor_;
    // std::shared_ptr<kcenon::monitoring::health::health_monitor> health_monitor_;
    // std::shared_ptr<kcenon::monitoring::collectors::thread_system_collector> thread_collector_;
    // std::shared_ptr<kcenon::monitoring::collectors::logger_system_collector> logger_collector_;
    // std::shared_ptr<kcenon::monitoring::collectors::system_resource_collector> resource_collector_;
    // std::shared_ptr<kcenon::monitoring::collectors::plugin_metric_collector> plugin_collector_;
    // std::shared_ptr<kcenon::monitoring::reliability::error_boundary> error_boundary_;
    // std::shared_ptr<kcenon::monitoring::reliability::fault_tolerance_manager> fault_tolerance_mgr_;
    // std::shared_ptr<kcenon::monitoring::reliability::retry_policy> retry_policy_;
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
