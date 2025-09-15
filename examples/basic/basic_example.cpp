/**
 * @file basic_example.cpp
 * @brief Basic example demonstrating integrated thread system usage
 *
 * This example shows how to use the integrated thread system for simple
 * task processing with automatic logging and monitoring.
 */

#include "application_framework.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>

using namespace kcenon::integrated;

int main() {
    std::cout << "=== Integrated Thread System - Basic Example ===" << std::endl;

    // Configure the application
    application_framework::application_config config;
    config.name = "Basic Example";
    config.version = "1.0.0";
    config.config_file_path = "config/development.json";
    config.auto_initialize = true;

    try {
        // Create application framework
        application_framework app(config);

        // Setup startup callback
        app.on_startup([&app]() -> result_void {
            auto logger = app.get_logger();
            logger->info("Basic example application starting...");
            return result_void{};
        });

        // Setup shutdown callback
        app.on_shutdown([&app]() -> result_void {
            auto logger = app.get_logger();
            logger->info("Basic example application shutting down...");
            return result_void{};
        });

        // Initialize the application
        auto init_result = app.initialize();
        if (!init_result) {
            std::cerr << "Failed to initialize application: "
                      << init_result.get_error().message() << std::endl;
            return 1;
        }

        // Get integrated components
        auto thread_pool = app.get_thread_pool();
        auto logger = app.get_logger();
        auto monitoring = app.get_monitoring();

        std::cout << "Application initialized successfully!" << std::endl;

        // Example 1: Simple task processing
        std::cout << "\n--- Example 1: Simple Tasks ---" << std::endl;

        std::vector<std::future<int>> futures;
        const int num_tasks = 10;

        for (int i = 0; i < num_tasks; ++i) {
            auto future_result = thread_pool->submit_task(
                "simple_calculation",
                [i]() -> int {
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    return i * i;
                }
            );

            if (future_result) {
                futures.push_back(std::move(future_result.value()));
                std::cout << "Submitted task " << i << std::endl;
            } else {
                std::cerr << "Failed to submit task " << i << ": "
                          << future_result.get_error().message() << std::endl;
            }
        }

        // Collect results
        std::cout << "\nTask results:" << std::endl;
        for (size_t i = 0; i < futures.size(); ++i) {
            try {
                int result = futures[i].get();
                std::cout << "Task " << i << " result: " << result << std::endl;
                logger->info("Task {} completed with result: {}", i, result);
            } catch (const std::exception& e) {
                std::cerr << "Task " << i << " failed: " << e.what() << std::endl;
                logger->error("Task {} failed: {}", i, e.what());
            }
        }

        // Example 2: Batch processing
        std::cout << "\n--- Example 2: Batch Processing ---" << std::endl;

        std::vector<int> data(100);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);

        for (auto& value : data) {
            value = dis(gen);
        }

        auto batch_futures = thread_pool->submit_batch(
            "data_processing",
            data.begin(),
            data.end(),
            [](int value) -> double {
                // Simulate data processing
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                return std::sqrt(value * value + 1.0);
            }
        );

        if (batch_futures) {
            std::cout << "Submitted batch of " << data.size() << " items" << std::endl;

            double total = 0.0;
            for (auto& future : batch_futures.value()) {
                total += future.get();
            }

            std::cout << "Batch processing completed. Total: " << total << std::endl;
            logger->info("Batch processing completed. Processed {} items, total: {}",
                        data.size(), total);
        } else {
            std::cerr << "Failed to submit batch: "
                      << batch_futures.get_error().message() << std::endl;
        }

        // Example 3: Performance monitoring
        std::cout << "\n--- Example 3: Performance Metrics ---" << std::endl;

        auto metrics = thread_pool->get_performance_metrics();
        std::cout << "Performance Metrics:" << std::endl;
        std::cout << "  Tasks submitted: " << metrics.tasks_submitted << std::endl;
        std::cout << "  Tasks completed: " << metrics.tasks_completed << std::endl;
        std::cout << "  Tasks failed: " << metrics.tasks_failed << std::endl;
        std::cout << "  Average latency: " << metrics.average_latency.count() << " ns" << std::endl;
        std::cout << "  Active workers: " << metrics.active_workers << std::endl;
        std::cout << "  Queue size: " << metrics.queue_size << std::endl;

        // Example 4: Health monitoring
        std::cout << "\n--- Example 4: Health Status ---" << std::endl;

        auto health = thread_pool->get_health_status();
        std::cout << "Health Status: ";
        switch (health.overall_health) {
            case health_status::level::healthy:
                std::cout << "Healthy ✅" << std::endl;
                break;
            case health_status::level::degraded:
                std::cout << "Degraded ⚠️" << std::endl;
                break;
            case health_status::level::critical:
                std::cout << "Critical ❌" << std::endl;
                break;
            case health_status::level::failed:
                std::cout << "Failed 💥" << std::endl;
                break;
        }

        std::cout << "CPU Usage: " << health.cpu_usage_percent << "%" << std::endl;
        std::cout << "Memory Usage: " << health.memory_usage_percent << "%" << std::endl;
        std::cout << "Queue Utilization: " << health.queue_utilization_percent << "%" << std::endl;

        if (!health.issues.empty()) {
            std::cout << "Issues:" << std::endl;
            for (const auto& issue : health.issues) {
                std::cout << "  - " << issue << std::endl;
            }
        }

        std::cout << "\n=== Example completed successfully! ===" << std::endl;

        // Log final message
        logger->info("Basic example completed successfully");

    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}