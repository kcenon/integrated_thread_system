// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/unified_thread_system.h>
#include <kcenon/integrated/core/system_coordinator.h>
#include <kcenon/integrated/core/configuration.h>
#include <kcenon/integrated/adapters/thread_adapter.h>
#include <kcenon/integrated/adapters/logger_adapter.h>
#include <kcenon/integrated/adapters/monitoring_adapter.h>
#include <kcenon/integrated/extensions/metrics_aggregator.h>
#include <kcenon/integrated/extensions/distributed_tracing.h>
#include <kcenon/integrated/extensions/plugin_manager.h>

#include <mutex>
#include <unordered_map>

namespace kcenon::integrated {

/**
 * @brief Implementation of unified_thread_system
 */
class unified_thread_system::impl {
public:
    explicit impl(const config& cfg)
        : config_(cfg)
        , shutting_down_(false) {

        // Convert old config to new unified_config
        unified_config unified_cfg;

        // Thread configuration
        unified_cfg.thread.name = cfg.name;
        unified_cfg.thread.thread_count = cfg.thread_count;
        unified_cfg.thread.max_queue_size = cfg.max_queue_size;
        unified_cfg.thread.enable_work_stealing = cfg.enable_work_stealing;
        unified_cfg.thread.enable_dynamic_scaling = cfg.enable_dynamic_scaling;
        unified_cfg.thread.min_threads = cfg.min_threads;
        unified_cfg.thread.max_threads = cfg.max_threads;

        // Logger configuration
        unified_cfg.logger.enable_file_logging = cfg.enable_file_logging;
        unified_cfg.logger.enable_console_logging = cfg.enable_console_logging;
        unified_cfg.logger.log_directory = cfg.log_directory;
        unified_cfg.logger.min_log_level = cfg.min_log_level;

        // Monitoring configuration
        unified_cfg.monitoring.enable_monitoring = cfg.enable_monitoring;

        // Circuit breaker configuration
        unified_cfg.circuit_breaker.enabled = cfg.enable_circuit_breaker;
        unified_cfg.circuit_breaker.failure_threshold = cfg.circuit_breaker_failure_threshold;
        unified_cfg.circuit_breaker.reset_timeout = cfg.circuit_breaker_reset_timeout;

        // Create coordinator
        coordinator_ = std::make_unique<system_coordinator>(unified_cfg);

        // Initialize extensions
        metrics_aggregator_ = std::make_unique<extensions::metrics_aggregator>();
        distributed_tracing_ = std::make_unique<extensions::distributed_tracing>();
        plugin_manager_ = std::make_unique<extensions::plugin_manager>();

        // Initialize all systems
        auto init_result = coordinator_->initialize();
        if (init_result.is_err()) {
            throw std::runtime_error("Failed to initialize unified_thread_system: " +
                                   init_result.error().message);
        }

        // Initialize extensions
        metrics_aggregator_->initialize();
        distributed_tracing_->initialize();
        plugin_manager_->initialize();
    }

    ~impl() {
        if (!shutting_down_) {
            shutdown_impl();
        }
    }

    void submit_internal(std::function<void()> task) {
        if (shutting_down_) {
            throw std::runtime_error("System is shutting down");
        }

        auto* thread_adapter = coordinator_->get_thread_adapter();
        if (!thread_adapter) {
            throw std::runtime_error("Thread adapter not available");
        }

        auto result = thread_adapter->execute(std::move(task));
        if (result.is_err()) {
            throw std::runtime_error("Failed to submit task: " + result.error().message);
        }
    }

    void submit_priority_internal(int priority, std::function<void()> task) {
        submit_internal(std::move(task));
    }

    void schedule_internal(std::chrono::milliseconds delay, std::function<void()> task) {
        auto* thread_adapter = coordinator_->get_thread_adapter();
        if (!thread_adapter) {
            throw std::runtime_error("Thread adapter not available");
        }

        thread_adapter->execute([delay, task = std::move(task)]() {
            std::this_thread::sleep_for(delay);
            task();
        });
    }

    size_t schedule_recurring_internal(std::chrono::milliseconds interval, std::function<void()> task) {
        return 0;
    }

    performance_metrics get_metrics() const {
        performance_metrics metrics;
        auto* thread_adapter = coordinator_->get_thread_adapter();
        if (thread_adapter) {
            metrics.active_workers = thread_adapter->worker_count();
            metrics.queue_size = thread_adapter->queue_size();
        }
        return metrics;
    }

    health_status get_health() const {
        health_status status;
        auto* monitoring_adapter = coordinator_->get_monitoring_adapter();
        if (monitoring_adapter) {
            auto health_result = monitoring_adapter->check_health();
            if (health_result.is_ok()) {
                status.overall_health = health_result.value().is_healthy() ?
                    health_level::healthy : health_level::degraded;
            }
        }
        return status;
    }

    void wait_for_completion() {
        auto* thread_adapter = coordinator_->get_thread_adapter();
        if (thread_adapter) {
            thread_adapter->wait_for_completion();
        }
    }

    bool wait_for_completion_timeout(std::chrono::milliseconds timeout) {
        auto* thread_adapter = coordinator_->get_thread_adapter();
        return thread_adapter ? thread_adapter->wait_for_completion_timeout(timeout) : true;
    }

    size_t worker_count() const {
        auto* thread_adapter = coordinator_->get_thread_adapter();
        return thread_adapter ? thread_adapter->worker_count() : 0;
    }

    void set_worker_count(size_t count) {}
    void set_work_stealing(bool enabled) {}

    size_t queue_size() const {
        auto* thread_adapter = coordinator_->get_thread_adapter();
        return thread_adapter ? thread_adapter->queue_size() : 0;
    }

    bool is_healthy() const {
        return get_health().overall_health == health_level::healthy;
    }

    void shutdown_impl() {
        if (shutting_down_) return;
        shutting_down_ = true;
        plugin_manager_->shutdown();
        distributed_tracing_->shutdown();
        metrics_aggregator_->shutdown();
        coordinator_->shutdown();
    }

    void shutdown_immediate() { shutdown_impl(); }
    bool is_shutting_down() const { return shutting_down_; }

    std::string export_metrics_json() const {
        return metrics_aggregator_->export_json_format();
    }

    std::string export_metrics_prometheus() const {
        return metrics_aggregator_->export_prometheus_format();
    }

    void cancel_recurring(size_t task_id) {}
    size_t subscribe_to_events(const std::string& event_type, event_callback callback) { return 0; }
    void unsubscribe_from_events(size_t subscription_id) {}
    void reset_circuit_breaker() {}
    bool is_circuit_open() const { return false; }

    void load_plugin(const std::string& plugin_path) {
        plugin_manager_->load_plugin(plugin_path);
    }

    void unload_plugin(const std::string& plugin_name) {
        plugin_manager_->unload_plugin(plugin_name);
    }

    std::vector<std::string> list_plugins() const {
        return plugin_manager_->list_plugins();
    }

    std::shared_ptr<void> create_cancellation_token() {
        auto* thread_adapter = coordinator_->get_thread_adapter();
        if (!thread_adapter) {
            throw std::runtime_error("Thread adapter not available");
        }
        return thread_adapter->create_cancellation_token();
    }

    void cancel_token(std::shared_ptr<void> token) {
        auto* thread_adapter = coordinator_->get_thread_adapter();
        if (!thread_adapter) {
            throw std::runtime_error("Thread adapter not available");
        }
        thread_adapter->cancel_token(token);
    }

    void submit_cancellable_internal(std::shared_ptr<void> token, std::function<void()> task) {
        if (shutting_down_) {
            throw std::runtime_error("System is shutting down");
        }

        auto* thread_adapter = coordinator_->get_thread_adapter();
        if (!thread_adapter) {
            throw std::runtime_error("Thread adapter not available");
        }

        // Submit via thread_adapter's cancel-aware submission
        auto future = thread_adapter->submit_cancellable(token, std::move(task));
        // Let the future go out of scope - we don't need to wait for it
        (void)future;
    }

private:
    config config_;
    std::atomic<bool> shutting_down_;

    std::unique_ptr<system_coordinator> coordinator_;
    std::unique_ptr<extensions::metrics_aggregator> metrics_aggregator_;
    std::unique_ptr<extensions::distributed_tracing> distributed_tracing_;
    std::unique_ptr<extensions::plugin_manager> plugin_manager_;
};

// unified_thread_system implementation

unified_thread_system::unified_thread_system(const config& cfg)
    : pimpl_(std::make_unique<impl>(cfg)) {}

unified_thread_system::~unified_thread_system() = default;

void unified_thread_system::submit_internal(std::function<void()> task) {
    pimpl_->submit_internal(std::move(task));
}

void unified_thread_system::submit_priority_internal(int priority, std::function<void()> task) {
    pimpl_->submit_priority_internal(priority, std::move(task));
}

void unified_thread_system::submit_cancellable_internal(std::shared_ptr<void> token, std::function<void()> task) {
    pimpl_->submit_cancellable_internal(token, std::move(task));
}

void unified_thread_system::schedule_internal(std::chrono::milliseconds delay, std::function<void()> task) {
    pimpl_->schedule_internal(delay, std::move(task));
}

size_t unified_thread_system::schedule_recurring_internal(std::chrono::milliseconds interval, std::function<void()> task) {
    return pimpl_->schedule_recurring_internal(interval, std::move(task));
}

performance_metrics unified_thread_system::get_metrics() const {
    return pimpl_->get_metrics();
}

health_status unified_thread_system::get_health() const {
    return pimpl_->get_health();
}

void unified_thread_system::wait_for_completion() {
    pimpl_->wait_for_completion();
}

bool unified_thread_system::wait_for_completion_timeout(std::chrono::milliseconds timeout) {
    return pimpl_->wait_for_completion_timeout(timeout);
}

size_t unified_thread_system::worker_count() const {
    return pimpl_->worker_count();
}

void unified_thread_system::set_worker_count(size_t count) {
    pimpl_->set_worker_count(count);
}

void unified_thread_system::set_work_stealing(bool enabled) {
    pimpl_->set_work_stealing(enabled);
}

size_t unified_thread_system::queue_size() const {
    return pimpl_->queue_size();
}

bool unified_thread_system::is_healthy() const {
    return pimpl_->is_healthy();
}

void unified_thread_system::shutdown() {
    pimpl_->shutdown_impl();
}

void unified_thread_system::shutdown_immediate() {
    pimpl_->shutdown_immediate();
}

bool unified_thread_system::is_shutting_down() const {
    return pimpl_->is_shutting_down();
}

std::string unified_thread_system::export_metrics_json() const {
    return pimpl_->export_metrics_json();
}

std::string unified_thread_system::export_metrics_prometheus() const {
    return pimpl_->export_metrics_prometheus();
}

void unified_thread_system::cancel_recurring(size_t task_id) {
    pimpl_->cancel_recurring(task_id);
}

std::shared_ptr<void> unified_thread_system::create_cancellation_token() {
    return pimpl_->create_cancellation_token();
}

void unified_thread_system::cancel_token(std::shared_ptr<void> token) {
    pimpl_->cancel_token(token);
}

size_t unified_thread_system::subscribe_to_events(const std::string& event_type, event_callback callback) {
    return pimpl_->subscribe_to_events(event_type, callback);
}

void unified_thread_system::unsubscribe_from_events(size_t subscription_id) {
    pimpl_->unsubscribe_from_events(subscription_id);
}

void unified_thread_system::reset_circuit_breaker() {
    pimpl_->reset_circuit_breaker();
}

bool unified_thread_system::is_circuit_open() const {
    return pimpl_->is_circuit_open();
}

void unified_thread_system::load_plugin(const std::string& plugin_path) {
    pimpl_->load_plugin(plugin_path);
}

void unified_thread_system::unload_plugin(const std::string& plugin_name) {
    pimpl_->unload_plugin(plugin_name);
}

std::vector<std::string> unified_thread_system::list_plugins() const {
    return pimpl_->list_plugins();
}

} // namespace kcenon::integrated
