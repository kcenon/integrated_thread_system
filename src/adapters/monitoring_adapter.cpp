// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/monitoring_adapter.h>

#if EXTERNAL_SYSTEMS_AVAILABLE
// Use external monitoring_system's performance monitor
#include <kcenon/monitoring/core/performance_monitor.h>

// New adapters and features (monitoring_system v2.0.0+)
#include <kcenon/monitoring/adaptive/adaptive_monitor.h>
#include <kcenon/monitoring/health/health_monitor.h>
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
            // Initialize monitoring_system v2.0.0+ components
            if (config_.enable_adaptive_monitoring) {
                adaptive_monitor_ = std::make_unique<kcenon::monitoring::adaptive_monitor>();
                auto result = adaptive_monitor_->start();
                if (!result.is_ok()) {
                    return common::VoidResult::err(
                        common::error_codes::INTERNAL_ERROR,
                        "Failed to start adaptive monitor"
                    );
                }
            }

            if (config_.enable_health_monitoring) {
                kcenon::monitoring::health_monitor_config health_cfg;
                health_cfg.check_interval = config_.health_check_interval;
                health_monitor_ = std::make_unique<kcenon::monitoring::health_monitor>(health_cfg);
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
                std::string("Monitoring adapter initialization failed: ") + e.what()
            );
        }
    }

    common::VoidResult shutdown() {
        if (!initialized_) {
            return common::ok();
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Clean up monitoring_system v2.0.0+ components
        if (adaptive_monitor_) {
            adaptive_monitor_->stop();
            adaptive_monitor_.reset();
        }

        if (health_monitor_) {
            health_monitor_.reset();
        }
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
        result.timestamp = std::chrono::system_clock::now();

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (health_monitor_) {
            auto health_result = health_monitor_->check_health();

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
        } else {
            result.status = common::interfaces::health_status::healthy;
            result.message = "Monitoring adapter is operational (health monitoring disabled)";
        }
#else
        result.status = common::interfaces::health_status::healthy;
        result.message = "Monitoring adapter is operational";
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
    // External monitoring_system integration
    // v2.0.0+ features
    std::unique_ptr<kcenon::monitoring::adaptive_monitor> adaptive_monitor_;
    std::unique_ptr<kcenon::monitoring::health_monitor> health_monitor_;
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
