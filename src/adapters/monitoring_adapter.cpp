// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/monitoring_adapter.h>

// TODO: Enable monitoring_system integration when dependencies are resolved
// #if EXTERNAL_SYSTEMS_AVAILABLE
// #include <kcenon/monitoring/core/performance_monitor.h>
// #endif

#include <mutex>
#include <unordered_map>
#include <chrono>

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
            // TODO: Integrate with actual monitoring_system
#endif

            // Fallback: Simple in-memory metrics storage
            initialized_ = true;
            start_time_ = std::chrono::steady_clock::now();
            return common::ok();
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

        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.clear();
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

        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_[name] = value;
        return common::ok();
    }

    common::VoidResult record_metric(const std::string& name, double value,
                                     const std::unordered_map<std::string, std::string>& tags) {
        // For simple implementation, ignore tags
        return record_metric(name, value);
    }

    common::Result<common::interfaces::metrics_snapshot> get_metrics() {
        if (!initialized_) {
            return common::Result<common::interfaces::metrics_snapshot>::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

        std::lock_guard<std::mutex> lock(metrics_mutex_);

        common::interfaces::metrics_snapshot snapshot;
        auto now = std::chrono::system_clock::now();

        // Convert metrics map to vector of metric_value
        for (const auto& [name, value] : metrics_) {
            common::interfaces::metric_value mv;
            mv.name = name;
            mv.value = value;
            mv.timestamp = now;
            snapshot.metrics.push_back(mv);
        }

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

        return common::Result<common::interfaces::health_check_result>::ok(result);
    }

    common::VoidResult reset() {
        if (!initialized_) {
            return common::VoidResult::err(
                common::error_codes::INVALID_ARGUMENT,
                "Monitoring adapter not initialized"
            );
        }

        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.clear();
        start_time_ = std::chrono::steady_clock::now();
        return common::ok();
    }

private:
    monitoring_config config_;
    bool initialized_;
    std::chrono::steady_clock::time_point start_time_;
    std::unordered_map<std::string, double> metrics_;
    mutable std::mutex metrics_mutex_;
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
