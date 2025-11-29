// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file adaptive_monitoring_demo.cpp
 * @brief Demonstrates adaptive monitoring features from monitoring_system v4.0.0
 *
 * This example shows:
 * - Load-based sampling rate adjustment
 * - Automatic interval adaptation
 * - Performance profiling with scoped timers
 * - System resource collection
 * - Metrics snapshot and export
 */

#include <kcenon/integrated/adapters/monitoring_adapter.h>
#include <kcenon/integrated/core/configuration.h>

#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

using namespace kcenon::integrated;
using namespace kcenon::integrated::adapters;
using namespace kcenon::common;
using namespace std::chrono_literals;

// Simulate variable workload
void simulate_workload(monitoring_adapter& monitor, std::atomic<bool>& running) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> work_dist(1, 10);
    std::uniform_int_distribution<> sleep_dist(10, 100);

    while (running) {
        // Use scoped timer for automatic performance tracking
        auto timer = monitor.time_operation("workload.process");

        // Simulate variable processing time
        int work_units = work_dist(gen);
        for (int i = 0; i < work_units * 1000; ++i) {
            volatile int x = i * i;
            (void)x;
        }

        // Random failure simulation
        if (work_dist(gen) > 8) {
            timer.mark_failed();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_dist(gen)));
    }
}

// Simulate high-load period
void simulate_high_load(std::atomic<bool>& running) {
    std::vector<std::thread> workers;
    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        workers.emplace_back([&running] {
            while (running) {
                volatile double x = 0;
                for (int j = 0; j < 100000; ++j) {
                    x += std::sin(j) * std::cos(j);
                }
                std::this_thread::sleep_for(1ms);
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }
}

void print_adaptation_stats(const monitoring_adapter& monitor) {
    auto stats_result = monitor.get_adaptation_stats();
    if (stats_result.is_ok()) {
        const auto& stats = stats_result.value();
        std::cout << "\n=== Adaptation Statistics ===\n";
        std::cout << std::format("Total adaptations: {}\n", stats.total_adaptations);
        std::cout << std::format("Upscale count: {}\n", stats.upscale_count);
        std::cout << std::format("Downscale count: {}\n", stats.downscale_count);
        std::cout << std::format("Samples collected: {}\n", stats.samples_collected);
        std::cout << std::format("Samples dropped: {}\n", stats.samples_dropped);
        std::cout << std::format("Current sampling rate: {:.2f}\n", stats.current_sampling_rate);
        std::cout << std::format("Current interval: {}ms\n", stats.current_interval.count());
        std::cout << std::format("Average CPU usage: {:.1f}%\n", stats.average_cpu_usage);
        std::cout << std::format("Average memory usage: {:.1f}%\n", stats.average_memory_usage);
    }
}

void print_system_resources(const monitoring_adapter& monitor) {
    auto resources_result = monitor.get_system_resources();
    if (resources_result.is_ok()) {
        const auto& res = resources_result.value();
        std::cout << "\n=== System Resources ===\n";
        std::cout << std::format("CPU usage: {:.1f}%\n", res.cpu_usage_percent);
        std::cout << std::format("Memory usage: {:.1f}%\n", res.memory_usage_percent);
        std::cout << std::format("Memory used: {} MB\n", res.memory_usage_bytes / (1024 * 1024));
        std::cout << std::format("Memory available: {} MB\n", res.available_memory_bytes / (1024 * 1024));
        std::cout << std::format("Thread count: {}\n", res.thread_count);
        std::cout << std::format("Disk read: {} KB/s\n", res.disk_read_bytes_per_sec / 1024);
        std::cout << std::format("Disk write: {} KB/s\n", res.disk_write_bytes_per_sec / 1024);
        std::cout << std::format("Network RX: {} KB/s\n", res.network_rx_bytes_per_sec / 1024);
        std::cout << std::format("Network TX: {} KB/s\n", res.network_tx_bytes_per_sec / 1024);
    }
}

void print_metrics_snapshot(monitoring_adapter& monitor) {
    auto metrics_result = monitor.get_metrics();
    if (metrics_result.is_ok()) {
        const auto& snapshot = metrics_result.value();
        std::cout << "\n=== Metrics Snapshot ===\n";
        std::cout << std::format("Source: {}\n", snapshot.source_id);
        std::cout << std::format("Metric count: {}\n", snapshot.metrics.size());

        // Print first 10 metrics
        int count = 0;
        for (const auto& metric : snapshot.metrics) {
            if (count++ >= 10) {
                std::cout << "... and more\n";
                break;
            }
            std::cout << std::format("  {}: {:.2f}\n", metric.name, metric.value);
        }
    }
}

int main() {
    std::cout << "=== Adaptive Monitoring Demo (monitoring_system v4.0.0) ===\n\n";

    // Configure monitoring with adaptive features enabled
    monitoring_config config;
    config.enable_monitoring = true;
    config.enable_adaptive_monitoring = true;
    config.enable_health_monitoring = true;
    config.enable_system_resource_collector = true;
    config.sampling_interval = std::chrono::milliseconds(500);

    // Adaptive thresholds
    config.adaptive_low_threshold = 0.3;   // 30% - increase sampling
    config.adaptive_high_threshold = 0.7;  // 70% - decrease sampling
    config.adaptive_min_interval = std::chrono::milliseconds(100);
    config.adaptive_max_interval = std::chrono::milliseconds(5000);

    // Resource thresholds
    config.cpu_threshold = 80.0;
    config.memory_threshold = 90.0;

    // Create and initialize adapter
    monitoring_adapter monitor(config);
    auto init_result = monitor.initialize();
    if (!init_result.is_ok()) {
        std::cerr << "Failed to initialize monitoring adapter\n";
        return 1;
    }

    std::cout << "Monitoring adapter initialized successfully.\n";

    // Register custom health checks
    monitor.register_health_check("demo_always_healthy", []() {
        return true;
    });

    std::atomic<bool> workload_running{true};
    std::atomic<bool> high_load_running{false};

    // Start workload simulation
    std::thread workload_thread([&]() {
        simulate_workload(monitor, workload_running);
    });

    // Phase 1: Normal load
    std::cout << "\n--- Phase 1: Normal Load (5 seconds) ---\n";
    std::this_thread::sleep_for(5s);
    print_adaptation_stats(monitor);
    print_system_resources(monitor);

    // Phase 2: High load - adaptive monitoring should reduce sampling
    std::cout << "\n--- Phase 2: High Load (5 seconds) ---\n";
    std::cout << "Starting CPU-intensive tasks...\n";
    high_load_running = true;
    std::thread high_load_thread([&]() {
        simulate_high_load(high_load_running);
    });

    std::this_thread::sleep_for(5s);
    print_adaptation_stats(monitor);
    print_system_resources(monitor);

    // Phase 3: Return to normal
    std::cout << "\n--- Phase 3: Return to Normal (5 seconds) ---\n";
    high_load_running = false;
    high_load_thread.join();

    std::this_thread::sleep_for(5s);
    print_adaptation_stats(monitor);
    print_system_resources(monitor);

    // Stop workload
    workload_running = false;
    workload_thread.join();

    // Print final metrics
    print_metrics_snapshot(monitor);

    // Check overall health
    auto health_result = monitor.check_health();
    if (health_result.is_ok()) {
        const auto& health = health_result.value();
        std::cout << "\n=== Health Status ===\n";
        std::cout << std::format("Status: {}\n",
            health.status == common::interfaces::health_status::healthy ? "HEALTHY" :
            health.status == common::interfaces::health_status::degraded ? "DEGRADED" : "UNHEALTHY");
        std::cout << std::format("Message: {}\n", health.message);
        std::cout << std::format("Check duration: {}ms\n", health.check_duration.count());

        for (const auto& [key, value] : health.metadata) {
            std::cout << std::format("  {}: {}\n", key, value);
        }
    }

    // Shutdown
    monitor.shutdown();
    std::cout << "\nMonitoring adapter shut down.\n";

    return 0;
}
