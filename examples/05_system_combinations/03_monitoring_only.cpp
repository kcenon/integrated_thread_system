/**
 * @file 03_monitoring_only.cpp
 * @brief Using only the monitoring_system without threading or logging
 * @description Pure metrics collection and health monitoring
 */

#include "unified_thread_system.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

using namespace integrated_thread_system;
using namespace std::chrono_literals;

/**
 * Monitoring-only configuration is ideal for:
 * - System health checkers
 * - Performance profilers
 * - Resource monitors
 * - Metrics collectors
 * - Dashboard backends
 */
class monitoring_only_demo {
private:
    unified_thread_system system_;

public:
    monitoring_only_demo() {
        // Configure for monitoring-only operation
        config cfg;
        cfg.enable_thread_system(false)      // Disable threading
           .enable_logger_system(false)       // Disable logging
           .enable_monitoring_system(true)    // Enable monitoring
           .set_metrics_interval(std::chrono::seconds(1))
           .set_metrics_retention(std::chrono::hours(24))
           .enable_system_metrics(true)
           .enable_custom_metrics(true);

        system_ = unified_thread_system(cfg);

        std::cout << "=== Monitoring-Only Configuration ===" << std::endl;
        std::cout << "✗ Thread System: DISABLED" << std::endl;
        std::cout << "✗ Logger System: DISABLED" << std::endl;
        std::cout << "✓ Monitoring System: ENABLED" << std::endl;
        std::cout << "Metrics interval: 1 second" << std::endl;
        std::cout << std::endl;
    }

    void demonstrate_system_metrics() {
        std::cout << "1. System Metrics Collection:" << std::endl;

        // Collect system metrics
        for (int i = 0; i < 5; ++i) {
            auto metrics = system_.get_system_metrics();

            std::cout << "   Sample " << i + 1 << ":" << std::endl;
            std::cout << "     CPU Usage: " << metrics.cpu_usage_percent << "%" << std::endl;
            std::cout << "     Memory: " << metrics.memory_used_mb << "MB / "
                      << metrics.memory_total_mb << "MB" << std::endl;
            std::cout << "     Disk I/O: " << metrics.disk_read_mb_per_sec << "MB/s read, "
                      << metrics.disk_write_mb_per_sec << "MB/s write" << std::endl;
            std::cout << "     Network: " << metrics.network_rx_mb_per_sec << "MB/s rx, "
                      << metrics.network_tx_mb_per_sec << "MB/s tx" << std::endl;

            std::this_thread::sleep_for(1s);
        }
    }

    void demonstrate_custom_metrics() {
        std::cout << "\n2. Custom Metrics Registration:" << std::endl;

        // Register custom metrics
        system_.register_metric("request_count", metric_type::counter);
        system_.register_metric("response_time_ms", metric_type::gauge);
        system_.register_metric("error_rate", metric_type::gauge);
        system_.register_metric("cache_hit_ratio", metric_type::gauge);

        // Simulate application activity
        for (int i = 0; i < 10; ++i) {
            // Increment request counter
            system_.increment_counter("request_count");

            // Set response time
            int response_time = 10 + (i * 5);
            system_.set_gauge("response_time_ms", response_time);

            // Set error rate
            double error_rate = (i % 3 == 0) ? 0.05 : 0.01;
            system_.set_gauge("error_rate", error_rate);

            // Set cache hit ratio
            double cache_ratio = 0.85 + (0.1 * std::sin(i));
            system_.set_gauge("cache_hit_ratio", cache_ratio);

            std::cout << "   Update " << i + 1 << ": ";
            std::cout << "requests=" << system_.get_counter("request_count") << ", ";
            std::cout << "response=" << response_time << "ms, ";
            std::cout << "errors=" << (error_rate * 100) << "%, ";
            std::cout << "cache=" << (cache_ratio * 100) << "%" << std::endl;

            std::this_thread::sleep_for(500ms);
        }
    }

    void demonstrate_health_checks() {
        std::cout << "\n3. Health Check System:" << std::endl;

        // Register health checks
        system_.register_health_check("database", [this]() {
            // Simulate database connectivity check
            std::this_thread::sleep_for(10ms);
            return health_status{true, "Connected to primary database"};
        });

        system_.register_health_check("cache", [this]() {
            // Simulate cache check
            return health_status{true, "Redis cache operational"};
        });

        system_.register_health_check("disk_space", [this]() {
            // Simulate disk space check
            auto metrics = system_.get_system_metrics();
            bool healthy = metrics.disk_free_percent > 10;
            std::string message = "Disk space: " +
                std::to_string(metrics.disk_free_percent) + "% free";
            return health_status{healthy, message};
        });

        // Run health checks
        for (int i = 0; i < 3; ++i) {
            auto health_report = system_.check_health();

            std::cout << "   Health Check #" << i + 1 << ":" << std::endl;
            std::cout << "     Overall: " << (health_report.is_healthy ? "HEALTHY" : "UNHEALTHY")
                      << std::endl;

            for (const auto& [component, status] : health_report.component_status) {
                std::cout << "     " << component << ": "
                          << (status.is_healthy ? "✓" : "✗") << " "
                          << status.message << std::endl;
            }

            std::this_thread::sleep_for(2s);
        }
    }

    void demonstrate_alerting() {
        std::cout << "\n4. Alerting System:" << std::endl;

        // Set up alert thresholds
        system_.set_alert_threshold("cpu_usage", 80.0, alert_severity::warning);
        system_.set_alert_threshold("cpu_usage", 95.0, alert_severity::critical);
        system_.set_alert_threshold("memory_usage_percent", 85.0, alert_severity::warning);
        system_.set_alert_threshold("error_rate", 0.05, alert_severity::warning);

        // Register alert callback
        system_.on_alert([](const alert& a) {
            std::cout << "   ALERT [" << to_string(a.severity) << "]: "
                      << a.metric_name << " = " << a.current_value
                      << " (threshold: " << a.threshold << ")" << std::endl;
        });

        // Simulate metrics that trigger alerts
        std::cout << "   Simulating high resource usage..." << std::endl;

        system_.set_gauge("cpu_usage", 75.0);
        std::this_thread::sleep_for(500ms);

        system_.set_gauge("cpu_usage", 85.0);  // Should trigger warning
        std::this_thread::sleep_for(500ms);

        system_.set_gauge("cpu_usage", 96.0);  // Should trigger critical
        std::this_thread::sleep_for(500ms);

        system_.set_gauge("error_rate", 0.08); // Should trigger warning
        std::this_thread::sleep_for(500ms);
    }

    void demonstrate_time_series() {
        std::cout << "\n5. Time Series Data:" << std::endl;

        // Enable time series collection for specific metrics
        system_.enable_time_series("response_time_ms", 60);  // Keep 60 data points

        // Generate time series data
        for (int i = 0; i < 20; ++i) {
            double response_time = 50 + 30 * std::sin(i * 0.5) + (rand() % 10);
            system_.set_gauge("response_time_ms", response_time);
            std::this_thread::sleep_for(100ms);
        }

        // Get time series data
        auto time_series = system_.get_time_series("response_time_ms", 10);  // Last 10 points

        std::cout << "   Response Time History (last 10 samples):" << std::endl;
        int index = 0;
        for (const auto& point : time_series) {
            std::cout << "     [" << index++ << "] " << point.value << "ms" << std::endl;
        }

        // Calculate statistics
        auto stats = system_.get_metric_statistics("response_time_ms");
        std::cout << "   Statistics:" << std::endl;
        std::cout << "     Min: " << stats.min << "ms" << std::endl;
        std::cout << "     Max: " << stats.max << "ms" << std::endl;
        std::cout << "     Avg: " << stats.average << "ms" << std::endl;
        std::cout << "     P95: " << stats.p95 << "ms" << std::endl;
        std::cout << "     P99: " << stats.p99 << "ms" << std::endl;
    }

    void demonstrate_export_formats() {
        std::cout << "\n6. Metrics Export Formats:" << std::endl;

        // Export to different formats
        std::cout << "   Prometheus format:" << std::endl;
        auto prometheus = system_.export_metrics(export_format::prometheus);
        std::cout << prometheus.substr(0, 200) << "..." << std::endl;

        std::cout << "\n   JSON format:" << std::endl;
        auto json = system_.export_metrics(export_format::json);
        std::cout << json.substr(0, 200) << "..." << std::endl;

        std::cout << "\n   StatsD format:" << std::endl;
        auto statsd = system_.export_metrics(export_format::statsd);
        std::cout << statsd.substr(0, 200) << "..." << std::endl;
    }

    void demonstrate_resource_tracking() {
        std::cout << "\n7. Resource Usage Tracking:" << std::endl;

        // Track resource usage over time
        std::vector<double> cpu_samples;
        std::vector<double> memory_samples;

        for (int i = 0; i < 10; ++i) {
            auto metrics = system_.get_system_metrics();
            cpu_samples.push_back(metrics.cpu_usage_percent);
            memory_samples.push_back(metrics.memory_used_mb);

            // Simulate some work to change metrics
            volatile double dummy = 0;
            for (int j = 0; j < 1000000; ++j) {
                dummy += std::sqrt(j);
            }

            std::this_thread::sleep_for(200ms);
        }

        // Calculate trends
        double cpu_avg = 0, mem_avg = 0;
        for (size_t i = 0; i < cpu_samples.size(); ++i) {
            cpu_avg += cpu_samples[i];
            mem_avg += memory_samples[i];
        }
        cpu_avg /= cpu_samples.size();
        mem_avg /= memory_samples.size();

        std::cout << "   Resource Usage Summary:" << std::endl;
        std::cout << "     Average CPU: " << cpu_avg << "%" << std::endl;
        std::cout << "     Average Memory: " << mem_avg << "MB" << std::endl;

        // Detect trends
        bool cpu_increasing = cpu_samples.back() > cpu_samples.front();
        bool mem_increasing = memory_samples.back() > memory_samples.front();

        std::cout << "     CPU Trend: " << (cpu_increasing ? "↑ Increasing" : "↓ Decreasing")
                  << std::endl;
        std::cout << "     Memory Trend: " << (mem_increasing ? "↑ Increasing" : "↓ Decreasing")
                  << std::endl;
    }

    void demonstrate_efficiency() {
        std::cout << "\n8. Monitoring Efficiency:" << std::endl;

        const int num_metrics = 1000;
        auto start = std::chrono::steady_clock::now();

        // Update many metrics
        for (int i = 0; i < num_metrics; ++i) {
            std::string metric_name = "metric_" + std::to_string(i);
            system_.set_gauge(metric_name, i * 1.5);
        }

        auto duration = std::chrono::steady_clock::now() - start;
        double metrics_per_second = num_metrics * 1000.0 /
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        std::cout << "   Metric update rate: " << static_cast<int>(metrics_per_second)
                  << " metrics/sec" << std::endl;
        std::cout << "   Lightweight monitoring overhead" << std::endl;
        std::cout << "   No threading or logging overhead" << std::endl;
    }

    void run_all_demonstrations() {
        demonstrate_system_metrics();
        demonstrate_custom_metrics();
        demonstrate_health_checks();
        demonstrate_alerting();
        demonstrate_time_series();
        demonstrate_export_formats();
        demonstrate_resource_tracking();
        demonstrate_efficiency();

        std::cout << "\n=== Monitoring-Only Benefits ===" << std::endl;
        std::cout << "✓ Lightweight metrics collection" << std::endl;
        std::cout << "✓ Real-time system health monitoring" << std::endl;
        std::cout << "✓ Multiple export formats" << std::endl;
        std::cout << "✓ Alert threshold management" << std::endl;
        std::cout << "✓ Time series analysis" << std::endl;
    }
};

int main() {
    try {
        monitoring_only_demo demo;
        demo.run_all_demonstrations();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * When to use Monitoring-Only configuration:
 *
 * 1. System Monitoring Tools
 *    - Health check endpoints
 *    - Metrics exporters
 *    - Performance profilers
 *
 * 2. Dashboard Backends
 *    - Grafana data sources
 *    - Prometheus exporters
 *    - Custom monitoring UIs
 *
 * 3. Resource Monitors
 *    - Container monitors
 *    - VM health checkers
 *    - Cloud resource trackers
 *
 * 4. Alert Systems
 *    - Threshold monitors
 *    - Anomaly detectors
 *    - SLA trackers
 */