// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/extensions/metrics_aggregator.h>
#include <kcenon/integrated/adapters/thread_adapter.h>
#include <kcenon/integrated/adapters/logger_adapter.h>
#include <kcenon/integrated/adapters/monitoring_adapter.h>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace kcenon::integrated::extensions {

class metrics_aggregator::impl {
public:
    impl()
        : initialized_(false)
        , thread_adapter_(nullptr)
        , logger_adapter_(nullptr)
        , monitoring_adapter_(nullptr)
        , tasks_submitted_(0)
        , tasks_completed_(0)
    {}

    common::VoidResult initialize() {
        initialized_ = true;
        return common::ok();
    }

    common::VoidResult shutdown() {
        initialized_ = false;
        thread_adapter_ = nullptr;
        logger_adapter_ = nullptr;
        monitoring_adapter_ = nullptr;
        return common::ok();
    }

    void set_thread_adapter(adapters::thread_adapter* adapter) {
        thread_adapter_ = adapter;
    }

    void set_logger_adapter(adapters::logger_adapter* adapter) {
        logger_adapter_ = adapter;
    }

    void set_monitoring_adapter(adapters::monitoring_adapter* adapter) {
        monitoring_adapter_ = adapter;
    }

    common::Result<aggregated_metrics> collect_metrics() {
        if (!initialized_) {
            return common::Result<aggregated_metrics>::err(
                common::error_codes::INVALID_ARGUMENT,
                "Metrics aggregator not initialized"
            );
        }

        aggregated_metrics metrics;
        metrics.timestamp = std::chrono::system_clock::now();

        // Collect thread pool metrics
        if (thread_adapter_ && thread_adapter_->is_initialized()) {
            metrics.thread_pool_workers = thread_adapter_->worker_count();
            metrics.thread_pool_queue_size = thread_adapter_->queue_size();
            metrics.tasks_submitted = tasks_submitted_.load();
            metrics.tasks_completed = tasks_completed_.load();
        }

        // Collect monitoring system metrics (CPU, memory)
        if (monitoring_adapter_ && monitoring_adapter_->is_initialized()) {
            auto mon_metrics_result = monitoring_adapter_->get_metrics();
            if (mon_metrics_result) {
                const auto& mon_metrics = mon_metrics_result.value();

                // Extract system metrics from monitoring adapter
                for (const auto& metric : mon_metrics.metrics) {
                    if (metric.name == "system.cpu_usage_percent") {
                        metrics.cpu_usage_percent = metric.value;
                    } else if (metric.name == "system.memory_usage_percent") {
                        metrics.memory_usage_percent = metric.value;
                    } else {
                        // Store other metrics in custom_metrics
                        metrics.custom_metrics[metric.name] = metric.value;
                    }
                }
            }
        }

        // Store latest metrics for Prometheus export
        latest_metrics_ = metrics;

        return common::Result<aggregated_metrics>::ok(metrics);
    }

    std::string export_prometheus_format() {
        std::ostringstream oss;

        // Header
        oss << "# HELP integrated_thread_system Metrics from Integrated Thread System\n";
        oss << "# TYPE integrated_thread_system gauge\n\n";

        // Thread pool metrics
        oss << "# HELP thread_pool_workers Number of worker threads\n";
        oss << "# TYPE thread_pool_workers gauge\n";
        oss << "thread_pool_workers " << latest_metrics_.thread_pool_workers << "\n\n";

        oss << "# HELP thread_pool_queue_size Current queue size\n";
        oss << "# TYPE thread_pool_queue_size gauge\n";
        oss << "thread_pool_queue_size " << latest_metrics_.thread_pool_queue_size << "\n\n";

        oss << "# HELP tasks_submitted_total Total tasks submitted\n";
        oss << "# TYPE tasks_submitted_total counter\n";
        oss << "tasks_submitted_total " << latest_metrics_.tasks_submitted << "\n\n";

        oss << "# HELP tasks_completed_total Total tasks completed\n";
        oss << "# TYPE tasks_completed_total counter\n";
        oss << "tasks_completed_total " << latest_metrics_.tasks_completed << "\n\n";

        // System metrics
        oss << "# HELP system_cpu_usage_percent CPU usage percentage\n";
        oss << "# TYPE system_cpu_usage_percent gauge\n";
        oss << "system_cpu_usage_percent " << latest_metrics_.cpu_usage_percent << "\n\n";

        oss << "# HELP system_memory_usage_percent Memory usage percentage\n";
        oss << "# TYPE system_memory_usage_percent gauge\n";
        oss << "system_memory_usage_percent " << latest_metrics_.memory_usage_percent << "\n\n";

        // Logger metrics
        oss << "# HELP log_messages_written_total Total log messages written\n";
        oss << "# TYPE log_messages_written_total counter\n";
        oss << "log_messages_written_total " << latest_metrics_.log_messages_written << "\n\n";

        oss << "# HELP log_errors_total Total log errors\n";
        oss << "# TYPE log_errors_total counter\n";
        oss << "log_errors_total " << latest_metrics_.log_errors << "\n\n";

        // Custom metrics
        for (const auto& [name, value] : latest_metrics_.custom_metrics) {
            oss << "# HELP " << name << " Custom metric\n";
            oss << "# TYPE " << name << " gauge\n";
            oss << name << " " << value << "\n\n";
        }

        return oss.str();
    }

    std::string export_json_format() {
        std::ostringstream oss;

        // Format timestamp as ISO 8601
        auto time_t_val = std::chrono::system_clock::to_time_t(latest_metrics_.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            latest_metrics_.timestamp.time_since_epoch()) % 1000;

        oss << "{\n";
        oss << "  \"timestamp\": \"" << std::put_time(std::gmtime(&time_t_val), "%Y-%m-%dT%H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z\",\n";

        oss << "  \"thread_pool\": {\n";
        oss << "    \"workers\": " << latest_metrics_.thread_pool_workers << ",\n";
        oss << "    \"queue_size\": " << latest_metrics_.thread_pool_queue_size << ",\n";
        oss << "    \"tasks_submitted\": " << latest_metrics_.tasks_submitted << ",\n";
        oss << "    \"tasks_completed\": " << latest_metrics_.tasks_completed << "\n";
        oss << "  },\n";

        oss << "  \"system\": {\n";
        oss << "    \"cpu_usage_percent\": " << latest_metrics_.cpu_usage_percent << ",\n";
        oss << "    \"memory_usage_percent\": " << latest_metrics_.memory_usage_percent << "\n";
        oss << "  },\n";

        oss << "  \"logger\": {\n";
        oss << "    \"messages_written\": " << latest_metrics_.log_messages_written << ",\n";
        oss << "    \"errors\": " << latest_metrics_.log_errors << "\n";
        oss << "  }";

        if (!latest_metrics_.custom_metrics.empty()) {
            oss << ",\n  \"custom_metrics\": {\n";
            bool first = true;
            for (const auto& [name, value] : latest_metrics_.custom_metrics) {
                if (!first) oss << ",\n";
                oss << "    \"" << name << "\": " << value;
                first = false;
            }
            oss << "\n  }";
        }

        oss << "\n}\n";
        return oss.str();
    }

    void increment_tasks_submitted() {
        tasks_submitted_.fetch_add(1, std::memory_order_relaxed);
    }

    void increment_tasks_completed() {
        tasks_completed_.fetch_add(1, std::memory_order_relaxed);
    }

private:
    bool initialized_;
    adapters::thread_adapter* thread_adapter_;
    adapters::logger_adapter* logger_adapter_;
    adapters::monitoring_adapter* monitoring_adapter_;

    // Counters (thread-safe)
    std::atomic<std::size_t> tasks_submitted_;
    std::atomic<std::size_t> tasks_completed_;

    // Latest collected metrics
    aggregated_metrics latest_metrics_;
};

metrics_aggregator::metrics_aggregator()
    : pimpl_(std::make_unique<impl>()) {}

metrics_aggregator::~metrics_aggregator() = default;

common::VoidResult metrics_aggregator::initialize() {
    return pimpl_->initialize();
}

common::VoidResult metrics_aggregator::shutdown() {
    return pimpl_->shutdown();
}

void metrics_aggregator::set_thread_adapter(adapters::thread_adapter* adapter) {
    pimpl_->set_thread_adapter(adapter);
}

void metrics_aggregator::set_logger_adapter(adapters::logger_adapter* adapter) {
    pimpl_->set_logger_adapter(adapter);
}

void metrics_aggregator::set_monitoring_adapter(adapters::monitoring_adapter* adapter) {
    pimpl_->set_monitoring_adapter(adapter);
}

common::Result<aggregated_metrics> metrics_aggregator::collect_metrics() {
    return pimpl_->collect_metrics();
}

std::string metrics_aggregator::export_prometheus_format() {
    return pimpl_->export_prometheus_format();
}

std::string metrics_aggregator::export_json_format() {
    return pimpl_->export_json_format();
}

} // namespace kcenon::integrated::extensions
