/**
 * @file unified_thread_system.cpp
 * @brief Implementation of the Unified Thread System with thread_adapter integration
 *
 * This implementation uses thread_adapter to leverage thread_system v2.0.0 features:
 * - typed_thread_pool for priority-based task execution
 * - work-stealing for improved load balancing
 * - service_registry for dependency injection
 */

#include <unified_thread_system.h>
#include <kcenon/integrated/adapters/thread_adapter.h>
#include <iostream>
#include <memory>
#include <future>
#include <thread>
#include <format>

namespace kcenon::integrated {

class unified_thread_system::impl {
private:
    config config_;
    std::unique_ptr<adapters::thread_adapter> thread_adapter_;
    bool initialized_ = false;

public:
    explicit impl(const config& cfg) : config_(cfg) {
        initialize();
    }

    ~impl() {
        shutdown();
    }

    void initialize() {
        if (initialized_) return;

        // Create thread_config from unified config
        thread_config thread_cfg;
        thread_cfg.name = config_.name;
        thread_cfg.pool_name = config_.name + "_pool";
        thread_cfg.thread_count = config_.thread_count;
        thread_cfg.enable_work_stealing = config_.enable_work_stealing;
        thread_cfg.enable_service_registry = config_.enable_service_registry;
        thread_cfg.enable_priority_scheduling = true;  // Enable for typed_thread_pool

        // Create and initialize thread adapter
        thread_adapter_ = std::make_unique<adapters::thread_adapter>(thread_cfg);
        auto result = thread_adapter_->initialize();

        if (result.is_ok()) {
            initialized_ = true;
            if (config_.enable_console_logging) {
                std::cout << std::format("[INFO] unified_thread_system initialized: {} with {} workers\n",
                    config_.name, worker_count());
            }
        } else {
            if (config_.enable_console_logging) {
                std::cerr << std::format("[ERROR] Failed to initialize thread system: {}\n",
                    result.error().message);
            }
        }
    }

    void shutdown() {
        if (!initialized_) return;

        if (thread_adapter_) {
            thread_adapter_->wait_for_completion();
            thread_adapter_->shutdown();
            thread_adapter_.reset();
        }

        initialized_ = false;
        if (config_.enable_console_logging) {
            std::cout << "[INFO] unified_thread_system shutdown complete\n";
        }
    }

    // Submit task with priority using thread_adapter
    void submit_with_priority(int priority, std::function<void()> task) {
        if (!initialized_ || !thread_adapter_) {
            // Fallback to std::async if not initialized
            std::thread([task = std::move(task)]() {
                try {
                    task();
                } catch (...) {
                    // Swallow exceptions
                }
            }).detach();
            return;
        }

        auto result = thread_adapter_->execute_with_priority(priority, std::move(task));
        if (result.is_err() && config_.enable_console_logging) {
            std::cerr << std::format("[WARNING] Task submission failed: {}\n",
                result.error().message);
        }
    }

    // Submit task without priority (uses default)
    void submit_internal(std::function<void()> task) {
        submit_with_priority(64, std::move(task));  // Default: normal priority
    }

    // Logger system operations
    void log_debug(const std::string& message) {
        if (config_.enable_console_logging) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
    }

    void log_info(const std::string& message) {
        if (config_.enable_console_logging) {
            std::cout << "[INFO] " << message << std::endl;
        }
    }

    void log_warning(const std::string& message) {
        if (config_.enable_console_logging) {
            std::cout << "[WARNING] " << message << std::endl;
        }
    }

    void log_error(const std::string& message) {
        if (config_.enable_console_logging) {
            std::cout << "[ERROR] " << message << std::endl;
        }
    }

    void log_critical(const std::string& message) {
        if (config_.enable_console_logging) {
            std::cout << "[CRITICAL] " << message << std::endl;
        }
    }

    // Monitor system operations
    performance_metrics get_system_metrics() const {
        performance_metrics metrics{};
        if (thread_adapter_) {
            metrics.active_workers = thread_adapter_->worker_count();
            metrics.queue_size = thread_adapter_->queue_size();
        }
        return metrics;
    }

    health_status check_health() const {
        health_status status{};
        status.overall_health = initialized_ ? health_level::healthy : health_level::failed;
        if (thread_adapter_) {
            status.queue_utilization_percent =
                static_cast<double>(thread_adapter_->queue_size()) / 10000.0 * 100.0;
        }
        return status;
    }

    // Configuration operations
    void reconfigure(const config& new_config) {
        shutdown();
        config_ = new_config;
        initialize();
    }

    config get_config() const {
        return config_;
    }

    size_t worker_count() const {
        return thread_adapter_ ? thread_adapter_->worker_count() : 0;
    }

    size_t queue_size() const {
        return thread_adapter_ ? thread_adapter_->queue_size() : 0;
    }

    void wait_for_completion() {
        if (thread_adapter_) {
            thread_adapter_->wait_for_completion();
        }
    }

    bool is_healthy() const {
        return initialized_ && thread_adapter_ && thread_adapter_->is_initialized();
    }

    // Internal logging
    template<typename... Args>
    void log_internal(log_level level, const std::string& message, Args&&... args) {
        if (!config_.enable_console_logging) return;

        std::string level_str;
        switch (level) {
            case log_level::trace: level_str = "TRACE"; break;
            case log_level::debug: level_str = "DEBUG"; break;
            case log_level::info: level_str = "INFO"; break;
            case log_level::warning: level_str = "WARNING"; break;
            case log_level::error: level_str = "ERROR"; break;
            case log_level::critical: level_str = "CRITICAL"; break;
        }

        std::cout << "[" << level_str << "] " << message << std::endl;
    }
};

// Constructor and destructor
unified_thread_system::unified_thread_system(const config& cfg)
    : pimpl_(std::make_unique<impl>(cfg)) {}

unified_thread_system::unified_thread_system()
    : pimpl_(std::make_unique<impl>(config{})) {}

unified_thread_system::~unified_thread_system() = default;

// Move operations
unified_thread_system::unified_thread_system(unified_thread_system&&) = default;
unified_thread_system& unified_thread_system::operator=(unified_thread_system&&) = default;

// Public method implementations
performance_metrics unified_thread_system::get_metrics() const {
    return pimpl_->get_system_metrics();
}

health_status unified_thread_system::get_health() const {
    return pimpl_->check_health();
}

void unified_thread_system::wait_for_completion() {
    pimpl_->wait_for_completion();
}

size_t unified_thread_system::worker_count() const {
    return pimpl_->worker_count();
}

size_t unified_thread_system::queue_size() const {
    return pimpl_->queue_size();
}

bool unified_thread_system::is_healthy() const {
    return pimpl_->is_healthy();
}

// Internal methods implementation
void unified_thread_system::submit_internal(std::function<void()> task) {
    pimpl_->submit_internal(std::move(task));
}

void unified_thread_system::submit_internal_with_priority(int priority, std::function<void()> task) {
    pimpl_->submit_with_priority(priority, std::move(task));
}

// Template method implementation
template<typename... Args>
void unified_thread_system::log(log_level level, const std::string& message, Args&&... args) {
    pimpl_->log_internal(level, message, std::forward<Args>(args)...);
}

// Explicit instantiation for common types
template void unified_thread_system::log<>(log_level, const std::string&);
template void unified_thread_system::log<int>(log_level, const std::string&, int&&);
template void unified_thread_system::log<double>(log_level, const std::string&, double&&);
template void unified_thread_system::log<std::string>(log_level, const std::string&, std::string&&);
template void unified_thread_system::log<const char*>(log_level, const std::string&, const char*&&);
template void unified_thread_system::log<size_t>(log_level, const std::string&, size_t&&);

} // namespace kcenon::integrated
