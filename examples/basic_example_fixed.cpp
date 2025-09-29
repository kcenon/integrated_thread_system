/**
 * @file basic_example_fixed.cpp
 * @brief Fixed basic example using correct unified_thread_system API
 *
 * This example shows how to use the integrated thread system for simple
 * task processing with automatic logging and monitoring.
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <numeric>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== Integrated Thread System - Basic Example (Fixed) ===" << std::endl;

    // Configure the system with simple config
    unified_thread_system::config config;
    config.name = "BasicExample";
    config.enable_console_logging = true;
    config.enable_file_logging = true;
    config.log_directory = "./logs";
    config.min_log_level = log_level::info;

    try {
        // Create unified thread system
        unified_thread_system system(config);

        std::cout << "Application initialized successfully!" << std::endl;

        // Example 1: Simple task processing
        std::cout << "\n--- Example 1: Simple Tasks ---" << std::endl;

        std::vector<std::future<int>> futures;
        const int num_tasks = 10;

        for (int i = 0; i < num_tasks; ++i) {
            auto future = system.submit([i]() -> int {
                // Simulate some work
                std::this_thread::sleep_for(100ms);
                return i * i;
            });

            futures.push_back(std::move(future));
            std::cout << "Submitted task " << i << std::endl;
        }

        // Collect results
        std::cout << "\nTask results:" << std::endl;
        for (size_t i = 0; i < futures.size(); ++i) {
            try {
                int result = futures[i].get();
                std::cout << "Task " << i << " result: " << result << std::endl;
                system.log(log_level::info, "Task " + std::to_string(i) +
                          " completed with result: " + std::to_string(result));
            } catch (const std::exception& e) {
                std::cerr << "Task " << i << " failed: " << e.what() << std::endl;
                system.log(log_level::error, "Task " + std::to_string(i) +
                          " failed: " + e.what());
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

        auto batch_futures = system.submit_batch(
            data.begin(),
            data.end(),
            [](int value) -> double {
                // Simulate data processing
                std::this_thread::sleep_for(1ms);
                return std::sqrt(value * value + 1.0);
            }
        );

        std::cout << "Submitted batch of " << data.size() << " items" << std::endl;

        double total = 0.0;
        for (auto& future : batch_futures) {
            total += future.get();
        }

        std::cout << "Batch processing completed. Total: " << total << std::endl;
        system.log(log_level::info, "Batch processing completed. Processed " +
                  std::to_string(data.size()) + " items, total: " + std::to_string(total));

        // Example 3: Performance monitoring
        std::cout << "\n--- Example 3: Performance Metrics ---" << std::endl;

        auto metrics = system.get_metrics();
        std::cout << "Performance Metrics:" << std::endl;
        std::cout << "  Tasks submitted: " << metrics.tasks_submitted << std::endl;
        std::cout << "  Tasks completed: " << metrics.tasks_completed << std::endl;
        std::cout << "  Tasks failed: " << metrics.tasks_failed << std::endl;
        std::cout << "  Average latency: " << metrics.average_latency.count() << " ns" << std::endl;
        std::cout << "  Active workers: " << metrics.active_workers << std::endl;
        std::cout << "  Queue size: " << metrics.queue_size << std::endl;

        // Example 4: Health monitoring
        std::cout << "\n--- Example 4: Health Status ---" << std::endl;

        auto health = system.get_health();
        std::cout << "Health Status: ";
        switch (health.overall_health) {
            case health_level::healthy:
                std::cout << "Healthy ✅" << std::endl;
                break;
            case health_level::degraded:
                std::cout << "Degraded ⚠️" << std::endl;
                break;
            case health_level::critical:
                std::cout << "Critical ❌" << std::endl;
                break;
            case health_level::failed:
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
        system.log(log_level::info, "Basic example completed successfully");

    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}