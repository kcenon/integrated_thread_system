#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <kcenon/thread/core/event_bus.h>
#include <kcenon/thread/core/configuration_manager.h>
#include <kcenon/thread/interfaces/shared_interfaces.h>
#include <kcenon/monitoring/core/performance_monitor.h>

namespace kcenon::integrated {

using namespace thread;
using namespace monitoring;

/**
 * @brief Aggregated metrics from all systems
 */
struct aggregated_metrics {
    std::chrono::system_clock::time_point timestamp;
    
    // Thread system metrics
    struct {
        std::size_t active_threads{0};
        std::size_t queued_tasks{0};
        std::size_t completed_tasks{0};
        double average_task_duration_ms{0.0};
        double thread_utilization{0.0};
    } thread_metrics;
    
    // Logger system metrics
    struct {
        std::size_t messages_logged{0};
        std::size_t errors_logged{0};
        std::size_t warnings_logged{0};
        double average_log_latency_ms{0.0};
        std::size_t buffer_usage_bytes{0};
    } logger_metrics;
    
    // Monitoring system metrics
    struct {
        double cpu_usage_percent{0.0};
        double memory_usage_mb{0.0};
        std::size_t memory_usage_bytes{0};
        double disk_io_mbps{0.0};
        double network_io_mbps{0.0};
    } system_metrics;
    
    // Performance metrics
    struct {
        double p50_latency_ms{0.0};
        double p95_latency_ms{0.0};
        double p99_latency_ms{0.0};
        double throughput_ops{0.0};
        double error_rate{0.0};
    } performance_metrics;
    
    // Plugin metrics
    struct {
        std::size_t loaded_plugins{0};
        std::size_t active_plugins{0};
        std::unordered_map<std::string, double> plugin_cpu_usage;
        std::unordered_map<std::string, double> plugin_memory_mb;
    } plugin_metrics;
};

/**
 * @brief Metrics collection event
 */
struct metrics_collected_event : event_base {
    aggregated_metrics metrics;
    std::string source;
    
    metrics_collected_event(aggregated_metrics m, std::string src)
        : metrics(std::move(m)), source(std::move(src)) {}
    
    std::string type_name() const override {
        return "MetricsCollectedEvent";
    }
};

/**
 * @brief Metrics threshold alert event
 */
struct metrics_alert_event : event_base {
    enum class alert_type {
        cpu_high,
        memory_high,
        error_rate_high,
        latency_high,
        throughput_low
    };
    
    alert_type type;
    std::string message;
    double current_value;
    double threshold;
    
    metrics_alert_event(alert_type t, std::string msg, double current, double thresh)
        : type(t), message(std::move(msg)), current_value(current), threshold(thresh) {}
    
    std::string type_name() const override {
        return "MetricsAlertEvent";
    }
};

/**
 * @brief Metrics aggregator for collecting metrics from all systems
 */
class metrics_aggregator {
public:
    /**
     * @brief Configuration for metrics aggregator
     */
    struct config {
        std::chrono::milliseconds collection_interval{1000};
        bool enable_thread_metrics{true};
        bool enable_logger_metrics{true};
        bool enable_system_metrics{true};
        bool enable_plugin_metrics{true};
        
        // Alert thresholds
        double cpu_threshold{80.0};
        double memory_threshold{80.0};
        double error_rate_threshold{5.0};
        double latency_p99_threshold{1000.0};
        double min_throughput{100.0};
        
        std::size_t max_history_size{3600}; // 1 hour at 1s interval
    };
    
    /**
     * @brief Constructor
     * @param cfg Configuration
     * @param bus Event bus for notifications
     */
    explicit metrics_aggregator(const config& cfg = {},
                                std::shared_ptr<event_bus> bus = nullptr)
        : config_(cfg), event_bus_(bus) {
        if (!event_bus_) {
            event_bus_ = std::make_shared<event_bus>();
        }
    }
    
    /**
     * @brief Destructor
     */
    ~metrics_aggregator() {
        stop();
    }
    
    /**
     * @brief Register a monitorable component
     * @param name Component name
     * @param component Monitorable component
     */
    void register_component(const std::string& name,
                           std::shared_ptr<shared::IMonitorable> component) {
        std::lock_guard<std::mutex> lock(components_mutex_);
        components_[name] = component;
    }
    
    /**
     * @brief Unregister a component
     * @param name Component name
     */
    void unregister_component(const std::string& name) {
        std::lock_guard<std::mutex> lock(components_mutex_);
        components_.erase(name);
    }
    
    /**
     * @brief Start metrics collection
     * @return True if started successfully
     */
    bool start() {
        if (running_.exchange(true)) {
            return false; // Already running
        }
        
        collection_thread_ = std::thread(&metrics_aggregator::collection_loop, this);
        return true;
    }
    
    /**
     * @brief Stop metrics collection
     */
    void stop() {
        if (running_.exchange(false)) {
            if (collection_thread_.joinable()) {
                collection_thread_.join();
            }
        }
    }
    
    /**
     * @brief Check if collecting metrics
     * @return True if running
     */
    bool is_running() const {
        return running_.load();
    }
    
    /**
     * @brief Get current aggregated metrics
     * @return Current metrics
     */
    aggregated_metrics get_current_metrics() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        if (!metrics_history_.empty()) {
            return metrics_history_.back();
        }
        return aggregated_metrics{};
    }
    
    /**
     * @brief Get metrics history
     * @param duration How far back to retrieve
     * @return Vector of historical metrics
     */
    std::vector<aggregated_metrics> get_history(
        std::chrono::seconds duration = std::chrono::seconds(3600)) const {
        
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto cutoff = now - duration;
        
        std::vector<aggregated_metrics> result;
        for (const auto& metrics : metrics_history_) {
            if (metrics.timestamp >= cutoff) {
                result.push_back(metrics);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Calculate average metrics over a time period
     * @param duration Time period
     * @return Average metrics
     */
    aggregated_metrics calculate_average(std::chrono::seconds duration) const {
        auto history = get_history(duration);
        if (history.empty()) {
            return aggregated_metrics{};
        }
        
        aggregated_metrics avg;
        avg.timestamp = std::chrono::system_clock::now();
        
        for (const auto& m : history) {
            avg.thread_metrics.active_threads += m.thread_metrics.active_threads;
            avg.thread_metrics.queued_tasks += m.thread_metrics.queued_tasks;
            avg.thread_metrics.completed_tasks += m.thread_metrics.completed_tasks;
            avg.thread_metrics.average_task_duration_ms += m.thread_metrics.average_task_duration_ms;
            avg.thread_metrics.thread_utilization += m.thread_metrics.thread_utilization;
            
            avg.system_metrics.cpu_usage_percent += m.system_metrics.cpu_usage_percent;
            avg.system_metrics.memory_usage_mb += m.system_metrics.memory_usage_mb;
            
            avg.performance_metrics.p95_latency_ms += m.performance_metrics.p95_latency_ms;
            avg.performance_metrics.p99_latency_ms += m.performance_metrics.p99_latency_ms;
            avg.performance_metrics.throughput_ops += m.performance_metrics.throughput_ops;
            avg.performance_metrics.error_rate += m.performance_metrics.error_rate;
        }
        
        std::size_t count = history.size();
        avg.thread_metrics.active_threads /= count;
        avg.thread_metrics.queued_tasks /= count;
        avg.thread_metrics.average_task_duration_ms /= count;
        avg.thread_metrics.thread_utilization /= count;
        
        avg.system_metrics.cpu_usage_percent /= count;
        avg.system_metrics.memory_usage_mb /= count;
        
        avg.performance_metrics.p95_latency_ms /= count;
        avg.performance_metrics.p99_latency_ms /= count;
        avg.performance_metrics.throughput_ops /= count;
        avg.performance_metrics.error_rate /= count;
        
        return avg;
    }
    
    /**
     * @brief Export metrics to JSON
     * @param metrics Metrics to export
     * @return JSON string
     */
    std::string export_to_json(const aggregated_metrics& metrics) const {
        // Simplified JSON export (would use nlohmann::json in production)
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"timestamp\": \"" << std::chrono::system_clock::to_time_t(metrics.timestamp) << "\",\n";
        ss << "  \"thread\": {\n";
        ss << "    \"active_threads\": " << metrics.thread_metrics.active_threads << ",\n";
        ss << "    \"queued_tasks\": " << metrics.thread_metrics.queued_tasks << ",\n";
        ss << "    \"completed_tasks\": " << metrics.thread_metrics.completed_tasks << ",\n";
        ss << "    \"average_task_duration_ms\": " << metrics.thread_metrics.average_task_duration_ms << ",\n";
        ss << "    \"thread_utilization\": " << metrics.thread_metrics.thread_utilization << "\n";
        ss << "  },\n";
        ss << "  \"system\": {\n";
        ss << "    \"cpu_usage_percent\": " << metrics.system_metrics.cpu_usage_percent << ",\n";
        ss << "    \"memory_usage_mb\": " << metrics.system_metrics.memory_usage_mb << "\n";
        ss << "  },\n";
        ss << "  \"performance\": {\n";
        ss << "    \"p95_latency_ms\": " << metrics.performance_metrics.p95_latency_ms << ",\n";
        ss << "    \"p99_latency_ms\": " << metrics.performance_metrics.p99_latency_ms << ",\n";
        ss << "    \"throughput_ops\": " << metrics.performance_metrics.throughput_ops << ",\n";
        ss << "    \"error_rate\": " << metrics.performance_metrics.error_rate << "\n";
        ss << "  }\n";
        ss << "}";
        return ss.str();
    }
    
    /**
     * @brief Get singleton instance
     * @return Metrics aggregator instance
     */
    static metrics_aggregator& instance() {
        static metrics_aggregator instance;
        return instance;
    }
    
private:
    /**
     * @brief Collection loop
     */
    void collection_loop() {
        while (running_) {
            auto metrics = collect_metrics();
            
            {
                std::lock_guard<std::mutex> lock(metrics_mutex_);
                if (metrics_history_.size() >= config_.max_history_size) {
                    metrics_history_.erase(metrics_history_.begin());
                }
                metrics_history_.push_back(metrics);
            }
            
            check_thresholds(metrics);
            
            // Publish metrics event
            if (event_bus_) {
                event_bus_->publish(metrics_collected_event(metrics, "aggregator"));
            }
            
            std::this_thread::sleep_for(config_.collection_interval);
        }
    }
    
    /**
     * @brief Collect metrics from all components
     */
    aggregated_metrics collect_metrics() {
        aggregated_metrics metrics;
        metrics.timestamp = std::chrono::system_clock::now();
        
        std::lock_guard<std::mutex> lock(components_mutex_);
        
        for (const auto& [name, component] : components_) {
            if (!component) continue;
            
            auto snapshot = component->get_metrics();
            
            // Aggregate based on component type
            if (name.find("thread") != std::string::npos) {
                metrics.thread_metrics.active_threads = snapshot.active_threads;
                metrics.thread_metrics.queued_tasks = snapshot.queued_tasks;
                metrics.thread_metrics.completed_tasks = snapshot.completed_tasks;
                metrics.thread_metrics.average_task_duration_ms = snapshot.average_task_duration_ms;
            }
            else if (name.find("monitor") != std::string::npos) {
                metrics.system_metrics.cpu_usage_percent = snapshot.cpu_usage;
                metrics.system_metrics.memory_usage_mb = snapshot.memory_usage_mb;
            }
            // Add more component types as needed
        }
        
        return metrics;
    }
    
    /**
     * @brief Check metrics against thresholds
     */
    void check_thresholds(const aggregated_metrics& metrics) {
        if (metrics.system_metrics.cpu_usage_percent > config_.cpu_threshold) {
            publish_alert(metrics_alert_event::alert_type::cpu_high,
                         "CPU usage exceeds threshold",
                         metrics.system_metrics.cpu_usage_percent,
                         config_.cpu_threshold);
        }
        
        if (metrics.system_metrics.memory_usage_mb > config_.memory_threshold) {
            publish_alert(metrics_alert_event::alert_type::memory_high,
                         "Memory usage exceeds threshold",
                         metrics.system_metrics.memory_usage_mb,
                         config_.memory_threshold);
        }
        
        if (metrics.performance_metrics.error_rate > config_.error_rate_threshold) {
            publish_alert(metrics_alert_event::alert_type::error_rate_high,
                         "Error rate exceeds threshold",
                         metrics.performance_metrics.error_rate,
                         config_.error_rate_threshold);
        }
        
        if (metrics.performance_metrics.p99_latency_ms > config_.latency_p99_threshold) {
            publish_alert(metrics_alert_event::alert_type::latency_high,
                         "P99 latency exceeds threshold",
                         metrics.performance_metrics.p99_latency_ms,
                         config_.latency_p99_threshold);
        }
        
        if (metrics.performance_metrics.throughput_ops < config_.min_throughput &&
            metrics.performance_metrics.throughput_ops > 0) {
            publish_alert(metrics_alert_event::alert_type::throughput_low,
                         "Throughput below minimum",
                         metrics.performance_metrics.throughput_ops,
                         config_.min_throughput);
        }
    }
    
    /**
     * @brief Publish alert event
     */
    void publish_alert(metrics_alert_event::alert_type type,
                      const std::string& message,
                      double current_value,
                      double threshold) {
        if (event_bus_) {
            event_bus_->publish(metrics_alert_event(type, message, current_value, threshold));
        }
    }
    
    config config_;
    std::shared_ptr<event_bus> event_bus_;
    std::atomic<bool> running_{false};
    std::thread collection_thread_;
    
    mutable std::mutex components_mutex_;
    std::unordered_map<std::string, std::shared_ptr<shared::IMonitorable>> components_;
    
    mutable std::mutex metrics_mutex_;
    std::vector<aggregated_metrics> metrics_history_;
};

} // namespace kcenon::integrated