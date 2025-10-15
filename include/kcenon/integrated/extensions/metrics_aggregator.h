// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file metrics_aggregator.h
 * @brief Aggregates metrics from all subsystems
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <kcenon/common/patterns/result.h>

namespace kcenon::integrated::extensions {

/**
 * @brief Aggregated metrics from all subsystems
 */
struct aggregated_metrics {
    // Thread metrics
    std::size_t thread_pool_workers{0};
    std::size_t thread_pool_queue_size{0};
    std::size_t tasks_submitted{0};
    std::size_t tasks_completed{0};

    // Logger metrics
    std::size_t log_messages_written{0};
    std::size_t log_errors{0};

    // System metrics
    double cpu_usage_percent{0.0};
    double memory_usage_percent{0.0};
    std::chrono::system_clock::time_point timestamp;

    // Custom metrics
    std::unordered_map<std::string, double> custom_metrics;
};

/**
 * @brief Metrics aggregator collects and combines metrics from all subsystems
 */
class metrics_aggregator {
public:
    metrics_aggregator();
    ~metrics_aggregator();

    metrics_aggregator(const metrics_aggregator&) = delete;
    metrics_aggregator& operator=(const metrics_aggregator&) = delete;

    common::VoidResult initialize();
    common::VoidResult shutdown();

    common::Result<aggregated_metrics> collect_metrics();
    std::string export_prometheus_format();
    std::string export_json_format();

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace kcenon::integrated::extensions
