// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/core/system_coordinator.h>
#include <kcenon/integrated/adapters/thread_adapter.h>
#include <kcenon/integrated/adapters/logger_adapter.h>
#include <kcenon/integrated/adapters/monitoring_adapter.h>

namespace kcenon::integrated {

/**
 * @brief Implementation details for system_coordinator
 */
class system_coordinator::impl {
public:
    explicit impl(const unified_config& config)
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

        // Initialize subsystems in dependency order
        // 1. Logger (no dependencies)
        // 2. Monitoring (may use logger)
        // 3. Thread pool (may use logger and monitoring)

        try {
            logger_adapter_ = std::make_unique<adapters::logger_adapter>(config_.logger);
            auto logger_result = logger_adapter_->initialize();
            if (logger_result.is_err()) {
                return logger_result;
            }

            monitoring_adapter_ = std::make_unique<adapters::monitoring_adapter>(config_.monitoring);
            auto monitoring_result = monitoring_adapter_->initialize();
            if (monitoring_result.is_err()) {
                return monitoring_result;
            }

            thread_adapter_ = std::make_unique<adapters::thread_adapter>(config_.thread);
            auto thread_result = thread_adapter_->initialize();
            if (thread_result.is_err()) {
                return thread_result;
            }

            initialized_ = true;
            return common::ok();
        } catch (const std::exception& e) {
            return common::VoidResult::err(
                common::error_codes::INTERNAL_ERROR,
                std::string("Failed to initialize subsystems: ") + e.what()
            );
        }
    }

    common::VoidResult shutdown() {
        if (!initialized_) {
            return common::ok();
        }

        // Shutdown in reverse order
        common::VoidResult result = common::ok();

        if (thread_adapter_) {
            auto thread_result = thread_adapter_->shutdown();
            if (thread_result.is_err()) {
                result = thread_result;  // Record error but continue shutdown
            }
            thread_adapter_.reset();
        }

        if (monitoring_adapter_) {
            auto monitoring_result = monitoring_adapter_->shutdown();
            if (monitoring_result.is_err() && result.is_ok()) {
                result = monitoring_result;
            }
            monitoring_adapter_.reset();
        }

        if (logger_adapter_) {
            auto logger_result = logger_adapter_->shutdown();
            if (logger_result.is_err() && result.is_ok()) {
                result = logger_result;
            }
            logger_adapter_.reset();
        }

        initialized_ = false;
        return result;
    }

    bool is_initialized() const {
        return initialized_;
    }

    adapters::thread_adapter* get_thread_adapter() {
        return thread_adapter_.get();
    }

    adapters::logger_adapter* get_logger_adapter() {
        return logger_adapter_.get();
    }

    adapters::monitoring_adapter* get_monitoring_adapter() {
        return monitoring_adapter_.get();
    }

private:
    unified_config config_;
    bool initialized_;

    std::unique_ptr<adapters::thread_adapter> thread_adapter_;
    std::unique_ptr<adapters::logger_adapter> logger_adapter_;
    std::unique_ptr<adapters::monitoring_adapter> monitoring_adapter_;
};

// system_coordinator implementation

system_coordinator::system_coordinator(const unified_config& config)
    : pimpl_(std::make_unique<impl>(config)) {
}

system_coordinator::~system_coordinator() = default;

system_coordinator::system_coordinator(system_coordinator&&) noexcept = default;
system_coordinator& system_coordinator::operator=(system_coordinator&&) noexcept = default;

common::VoidResult system_coordinator::initialize() {
    return pimpl_->initialize();
}

common::VoidResult system_coordinator::shutdown() {
    return pimpl_->shutdown();
}

bool system_coordinator::is_initialized() const {
    return pimpl_->is_initialized();
}

adapters::thread_adapter* system_coordinator::get_thread_adapter() {
    return pimpl_->get_thread_adapter();
}

adapters::logger_adapter* system_coordinator::get_logger_adapter() {
    return pimpl_->get_logger_adapter();
}

adapters::monitoring_adapter* system_coordinator::get_monitoring_adapter() {
    return pimpl_->get_monitoring_adapter();
}

} // namespace kcenon::integrated
