/**
 * @file 07_all_systems.cpp
 * @brief Using all three systems together: thread, logger, and monitoring
 * @description Complete integrated system with all features enabled
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

/**
 * All-systems configuration is ideal for:
 * - Production application servers
 * - Enterprise microservices
 * - Cloud-native applications
 * - Mission-critical systems
 * - Full-featured application platforms
 */
class all_systems_demo {
private:
    unified_thread_system system_;
    std::atomic<int> requests_processed_{0};
    std::atomic<int> errors_encountered_{0};

public:
    all_systems_demo() {
        // Configure with all systems enabled
        config cfg;
        cfg.enable_thread_system(true)       // Enable threading
           .enable_logger_system(true)        // Enable logging
           .enable_monitoring_system(true)    // Enable monitoring
           .set_worker_count(std::thread::hardware_concurrency())
           .set_queue_capacity(10000)
           .enable_work_stealing(true)
           .enable_adaptive_optimization(true)
           // Logging configuration
           .set_log_level(log_level::debug)
           .set_log_file("production.log")
           .enable_async_logging(true)
           .set_log_rotation_size(100 * 1024 * 1024)  // 100MB
           // Monitoring configuration
           .set_metrics_interval(std::chrono::seconds(1))
           .enable_system_metrics(true)
           .enable_custom_metrics(true)
           .set_alert_threshold("cpu_usage", 80.0, alert_severity::warning)
           .set_alert_threshold("memory_usage", 85.0, alert_severity::warning)
           .set_alert_threshold("error_rate", 0.05, alert_severity::critical);

        system_ = unified_thread_system(cfg);

        // Register custom metrics
        system_.register_metric("request_count", metric_type::counter);
        system_.register_metric("request_latency_ms", metric_type::gauge);
        system_.register_metric("error_count", metric_type::counter);
        system_.register_metric("queue_depth", metric_type::gauge);
        system_.register_metric("active_workers", metric_type::gauge);

        // Register health checks
        system_.register_health_check("thread_pool", [this]() {
            auto queue_depth = system_.get_queue_depth();
            bool healthy = queue_depth < 5000;
            return health_status{healthy,
                "Queue depth: " + std::to_string(queue_depth)};
        });

        system_.register_health_check("error_rate", [this]() {
            double rate = (requests_processed_ > 0) ?
                static_cast<double>(errors_encountered_) / requests_processed_ : 0;
            bool healthy = rate < 0.05;
            return health_status{healthy,
                "Error rate: " + std::to_string(rate * 100) + "%"};
        });

        std::cout << "=== All Systems Configuration ===" << std::endl;
        std::cout << "✓ Thread System: ENABLED (" << std::thread::hardware_concurrency()
                  << " workers)" << std::endl;
        std::cout << "✓ Logger System: ENABLED (async)" << std::endl;
        std::cout << "✓ Monitoring System: ENABLED (1s interval)" << std::endl;
        std::cout << std::endl;
    }

    void demonstrate_production_server() {
        std::cout << "1. Production Server Simulation:" << std::endl;

        system_.log_info("Production server starting",
            {{"version", "1.0.0"},
             {"environment", "production"},
             {"workers", std::thread::hardware_concurrency()}});

        // Simulate incoming requests
        std::atomic<bool> server_running{true};
        std::vector<std::thread> client_simulators;

        // Start client simulators
        for (int client_id = 0; client_id < 3; ++client_id) {
            client_simulators.emplace_back([this, client_id, &server_running]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> delay_dist(10, 100);
                std::uniform_int_distribution<> type_dist(0, 3);

                while (server_running) {
                    // Generate request
                    auto request_type = type_dist(gen);
                    auto request_id = requests_processed_.fetch_add(1);

                    // Submit request based on type
                    switch (request_type) {
                        case 0:  // Health check (critical)
                            handle_health_check_request(request_id);
                            break;
                        case 1:  // API call (normal)
                            handle_api_request(request_id, client_id);
                            break;
                        case 2:  // Background task
                            handle_background_task(request_id);
                            break;
                        case 3:  // Data processing
                            handle_data_processing(request_id);
                            break;
                    }

                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(delay_dist(gen)));
                }
            });
        }

        // Monitor server metrics
        std::thread monitor([this, &server_running]() {
            while (server_running) {
                // Update metrics
                system_.set_gauge("queue_depth", system_.get_queue_depth());
                system_.set_gauge("active_workers", system_.get_active_worker_count());

                // Calculate error rate
                double error_rate = (requests_processed_ > 0) ?
                    static_cast<double>(errors_encountered_) / requests_processed_ : 0;
                system_.set_gauge("error_rate", error_rate);

                // Check health
                auto health = system_.check_health();
                if (!health.is_healthy) {
                    system_.log_warning("Health check failed",
                        {{"healthy", false},
                         {"components", health.component_status.size()}});
                }

                // Get system metrics
                auto metrics = system_.get_system_metrics();
                system_.set_gauge("cpu_usage", metrics.cpu_usage_percent);
                system_.set_gauge("memory_usage", metrics.memory_used_mb);

                std::this_thread::sleep_for(1s);
            }
        });

        // Run server for demonstration period
        std::this_thread::sleep_for(5s);
        server_running = false;

        // Cleanup
        for (auto& t : client_simulators) t.join();
        monitor.join();

        // Print summary
        std::cout << "   Server processed " << requests_processed_.load() << " requests" << std::endl;
        std::cout << "   Errors encountered: " << errors_encountered_.load() << std::endl;
        std::cout << "   Error rate: "
                  << (static_cast<double>(errors_encountered_) / requests_processed_ * 100)
                  << "%" << std::endl;

        system_.log_info("Server shutdown complete",
            {{"total_requests", requests_processed_.load()},
             {"total_errors", errors_encountered_.load()}});
    }

    void handle_health_check_request(int request_id) {
        auto future = system_.submit_critical([this, request_id]() {
            auto start = std::chrono::steady_clock::now();

            system_.log_debug("Health check request {}", request_id);

            // Quick health check
            std::this_thread::sleep_for(5ms);

            auto duration = std::chrono::steady_clock::now() - start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            system_.set_gauge("request_latency_ms", ms);
            system_.increment_counter("request_count");

            system_.log_info("Health check completed",
                {{"request_id", request_id},
                 {"latency_ms", ms}});

            return true;
        });
    }

    void handle_api_request(int request_id, int client_id) {
        auto future = system_.submit([this, request_id, client_id]() {
            auto start = std::chrono::steady_clock::now();

            system_.log_info("API request received",
                {{"request_id", request_id},
                 {"client_id", client_id}});

            // Simulate API processing
            std::this_thread::sleep_for(50ms);

            // Simulate occasional errors
            if (request_id % 50 == 0) {
                errors_encountered_.fetch_add(1);
                system_.increment_counter("error_count");
                system_.log_error("API request failed",
                    {{"request_id", request_id},
                     {"error", "Internal server error"}});
                throw std::runtime_error("Simulated error");
            }

            auto duration = std::chrono::steady_clock::now() - start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            system_.set_gauge("request_latency_ms", ms);
            system_.increment_counter("request_count");

            system_.log_info("API request completed",
                {{"request_id", request_id},
                 {"client_id", client_id},
                 {"latency_ms", ms}});

            return std::string("response_" + std::to_string(request_id));
        });
    }

    void handle_background_task(int request_id) {
        auto future = system_.submit_background([this, request_id]() {
            system_.log_debug("Background task started",
                {{"request_id", request_id}});

            // Simulate background work
            std::this_thread::sleep_for(100ms);

            system_.increment_counter("request_count");

            system_.log_debug("Background task completed",
                {{"request_id", request_id}});
        });
    }

    void handle_data_processing(int request_id) {
        auto future = system_.submit([this, request_id]() {
            auto start = std::chrono::steady_clock::now();

            system_.log_info("Data processing started",
                {{"request_id", request_id}});

            // Simulate data processing
            std::vector<int> data(1000);
            std::iota(data.begin(), data.end(), 0);

            int result = std::accumulate(data.begin(), data.end(), 0);

            auto duration = std::chrono::steady_clock::now() - start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            system_.set_gauge("request_latency_ms", ms);
            system_.increment_counter("request_count");

            system_.log_info("Data processing completed",
                {{"request_id", request_id},
                 {"result", result},
                 {"latency_ms", ms}});

            return result;
        });
    }

    void demonstrate_performance_analysis() {
        std::cout << "\n2. Performance Analysis with All Systems:" << std::endl;

        system_.log_info("Starting performance analysis");

        // Enable detailed monitoring
        system_.enable_time_series("request_latency_ms", 100);
        system_.enable_time_series("queue_depth", 100);

        // Generate load
        const int num_requests = 100;
        std::vector<std::future<double>> futures;

        auto load_start = std::chrono::steady_clock::now();

        for (int i = 0; i < num_requests; ++i) {
            auto priority = (i % 10 == 0) ? job_priority::critical :
                          (i % 3 == 0) ? job_priority::background :
                          job_priority::normal;

            futures.push_back(system_.submit_with_priority(priority, [this, i]() {
                auto task_start = std::chrono::steady_clock::now();

                // Log task start
                system_.log_debug("Task {} started", i);

                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(10 + (i % 20)));

                auto duration = std::chrono::steady_clock::now() - task_start;
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

                // Update metrics
                system_.set_gauge("request_latency_ms", ms);

                // Log completion
                system_.log_debug("Task {} completed in {}ms", i, ms);

                return static_cast<double>(ms);
            }));
        }

        // Collect results
        double total_latency = 0;
        for (auto& future : futures) {
            total_latency += future.get();
        }

        auto total_duration = std::chrono::steady_clock::now() - load_start;
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_duration).count();

        // Get statistics
        auto stats = system_.get_metric_statistics("request_latency_ms");

        system_.log_info("Performance analysis complete",
            {{"total_requests", num_requests},
             {"total_time_ms", total_ms},
             {"avg_latency_ms", total_latency / num_requests},
             {"min_latency_ms", stats.min},
             {"max_latency_ms", stats.max},
             {"p95_latency_ms", stats.p95},
             {"p99_latency_ms", stats.p99}});

        std::cout << "   Processed " << num_requests << " requests in " << total_ms << "ms" << std::endl;
        std::cout << "   Average latency: " << (total_latency / num_requests) << "ms" << std::endl;
        std::cout << "   P95 latency: " << stats.p95 << "ms" << std::endl;
        std::cout << "   P99 latency: " << stats.p99 << "ms" << std::endl;
        std::cout << "   Throughput: " << (num_requests * 1000.0 / total_ms) << " req/sec" << std::endl;
    }

    void demonstrate_adaptive_behavior() {
        std::cout << "\n3. Adaptive System Behavior:" << std::endl;

        system_.log_info("Demonstrating adaptive behavior");

        // Phase 1: Low load
        std::cout << "   Phase 1: Low load..." << std::endl;
        system_.log_info("Phase 1: Low load");

        for (int i = 0; i < 10; ++i) {
            system_.submit([this, i]() {
                system_.log_debug("Low load task {}", i);
                std::this_thread::sleep_for(50ms);
            });
            std::this_thread::sleep_for(100ms);
        }

        auto low_load_metrics = system_.get_performance_stats();
        std::cout << "     Queue strategy: " << low_load_metrics.queue_strategy << std::endl;

        // Phase 2: High load
        std::cout << "   Phase 2: High load..." << std::endl;
        system_.log_info("Phase 2: High load");

        std::vector<std::future<void>> high_load_futures;
        for (int i = 0; i < 500; ++i) {
            high_load_futures.push_back(system_.submit([this, i]() {
                system_.log_debug("High load task {}", i);
                // Quick task to create contention
            }));
        }

        for (auto& f : high_load_futures) f.wait();

        auto high_load_metrics = system_.get_performance_stats();
        std::cout << "     Queue strategy: " << high_load_metrics.queue_strategy << std::endl;

        system_.log_info("Adaptive behavior demonstration complete");
    }

    void demonstrate_full_observability() {
        std::cout << "\n4. Full System Observability:" << std::endl;

        system_.log_info("Demonstrating full observability");

        // Get comprehensive system state
        auto thread_stats = system_.get_thread_statistics();
        auto logger_stats = system_.get_logger_statistics();
        auto monitor_stats = system_.get_monitor_statistics();

        std::cout << "   Thread System:" << std::endl;
        std::cout << "     Workers: " << thread_stats.worker_count << std::endl;
        std::cout << "     Queue depth: " << thread_stats.queue_depth << std::endl;
        std::cout << "     Tasks processed: " << thread_stats.total_tasks_processed << std::endl;

        std::cout << "   Logger System:" << std::endl;
        std::cout << "     Logs written: " << logger_stats.total_logs_written << std::endl;
        std::cout << "     Log file size: " << logger_stats.log_file_size_mb << "MB" << std::endl;
        std::cout << "     Async queue: " << logger_stats.async_queue_depth << std::endl;

        std::cout << "   Monitoring System:" << std::endl;
        std::cout << "     Metrics registered: " << monitor_stats.metrics_count << std::endl;
        std::cout << "     Health checks: " << monitor_stats.health_checks_count << std::endl;
        std::cout << "     Alerts triggered: " << monitor_stats.alerts_triggered << std::endl;

        // Export metrics in various formats
        std::cout << "\n   Exporting metrics..." << std::endl;
        auto prometheus_export = system_.export_metrics(export_format::prometheus);
        auto json_export = system_.export_metrics(export_format::json);

        system_.log_info("Metrics exported",
            {{"prometheus_size", prometheus_export.size()},
             {"json_size", json_export.size()}});

        std::cout << "     Prometheus format: " << prometheus_export.size() << " bytes" << std::endl;
        std::cout << "     JSON format: " << json_export.size() << " bytes" << std::endl;
    }

    void demonstrate_production_patterns() {
        std::cout << "\n5. Production Patterns:" << std::endl;

        // Circuit breaker pattern
        system_.log_info("Demonstrating circuit breaker pattern");

        auto circuit_breaker_task = [this](int id) {
            static std::atomic<int> failure_count{0};
            static std::atomic<bool> circuit_open{false};

            return system_.submit([id, &failure_count, &circuit_open, this]() {
                if (circuit_open.load()) {
                    system_.log_warning("Circuit breaker open for task {}", id);
                    throw std::runtime_error("Circuit breaker open");
                }

                try {
                    // Simulate work with potential failure
                    if (id % 10 == 0) {
                        throw std::runtime_error("Simulated failure");
                    }

                    failure_count = 0;
                    system_.log_debug("Task {} succeeded", id);
                    return id * 2;

                } catch (const std::exception& e) {
                    int failures = failure_count.fetch_add(1) + 1;

                    system_.log_error("Task {} failed: {}", id, e.what());
                    system_.increment_counter("error_count");

                    if (failures >= 3) {
                        circuit_open = true;
                        system_.log_critical("Circuit breaker opened after {} failures", failures);
                    }

                    throw;
                }
            });
        };

        // Test circuit breaker
        for (int i = 0; i < 15; ++i) {
            try {
                auto future = circuit_breaker_task(i);
                int result = future.get();
                std::cout << "     Task " << i << " result: " << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "     Task " << i << " failed: " << e.what() << std::endl;
            }
        }
    }

    void run_all_demonstrations() {
        demonstrate_production_server();
        demonstrate_performance_analysis();
        demonstrate_adaptive_behavior();
        demonstrate_full_observability();
        demonstrate_production_patterns();

        std::cout << "\n=== All Systems Benefits ===" << std::endl;
        std::cout << "✓ Complete observability (logs + metrics + health)" << std::endl;
        std::cout << "✓ Adaptive performance optimization" << std::endl;
        std::cout << "✓ Production-ready error handling" << std::endl;
        std::cout << "✓ Full audit trail with performance metrics" << std::endl;
        std::cout << "✓ Real-time alerting and monitoring" << std::endl;
        std::cout << "✓ Enterprise-grade reliability" << std::endl;

        // Final statistics
        auto final_stats = system_.get_comprehensive_statistics();
        std::cout << "\nFinal Statistics:" << std::endl;
        std::cout << "  Total requests: " << requests_processed_.load() << std::endl;
        std::cout << "  Total errors: " << errors_encountered_.load() << std::endl;
        std::cout << "  Error rate: "
                  << (static_cast<double>(errors_encountered_) / requests_processed_ * 100)
                  << "%" << std::endl;
    }
};

int main() {
    try {
        all_systems_demo demo;
        demo.run_all_demonstrations();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * When to use All-Systems configuration:
 *
 * 1. Production Applications
 *    - Enterprise services
 *    - Mission-critical systems
 *    - Financial applications
 *
 * 2. Cloud-Native Services
 *    - Kubernetes deployments
 *    - Microservices
 *    - Serverless functions
 *
 * 3. Monitoring-Critical Systems
 *    - SLA-bound services
 *    - High-availability systems
 *    - Real-time applications
 *
 * 4. Development & Testing
 *    - Full integration testing
 *    - Performance profiling
 *    - System debugging
 */