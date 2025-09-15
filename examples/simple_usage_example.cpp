/**
 * @file simple_usage_example.cpp
 * @brief Demonstrates the simplified usage that matches original thread_system
 *
 * This example shows how the unified API provides the same ease of use
 * as the original thread_system before it was split into 3 components.
 */

#include "../include/unified_thread_system.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>

using namespace kcenon::integrated;

int main() {
    std::cout << "=== Unified Thread System - Simple Usage ===\n\n";

    // ========================================
    // ORIGINAL THREAD_SYSTEM STYLE USAGE
    // ========================================

    std::cout << "Creating thread system (auto-configured)...\n";

    // Single line creation - matches original simplicity
    unified_thread_system system;

    std::cout << "System created with " << system.worker_count()
              << " workers and automatic logging/monitoring\n\n";

    // ========================================
    // EXAMPLE 1: Simple task submission
    // ========================================

    std::cout << "--- Example 1: Simple Tasks ---\n";

    // Submit individual tasks (same as original thread_system)
    auto future1 = system.submit([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 42;
    });

    auto future2 = system.submit([](int x, int y) -> int {
        return x + y;
    }, 10, 20);

    auto future3 = system.submit([]() -> std::string {
        return "Hello from worker thread!";
    });

    // Get results
    std::cout << "Task 1 result: " << future1.get() << "\n";
    std::cout << "Task 2 result: " << future2.get() << "\n";
    std::cout << "Task 3 result: " << future3.get() << "\n\n";

    // ========================================
    // EXAMPLE 2: Batch processing
    // ========================================

    std::cout << "--- Example 2: Batch Processing ---\n";

    std::vector<int> numbers(20);
    std::iota(numbers.begin(), numbers.end(), 1); // Fill with 1,2,3...20

    // Process all numbers in parallel
    auto batch_futures = system.submit_batch(
        numbers.begin(),
        numbers.end(),
        [](int n) -> int {
            // Simulate some computation
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return n * n;
        }
    );

    // Collect results
    std::vector<int> results;
    for (auto& future : batch_futures) {
        results.push_back(future.get());
    }

    int sum = std::accumulate(results.begin(), results.end(), 0);
    std::cout << "Processed " << numbers.size() << " items in parallel\n";
    std::cout << "Sum of squares: " << sum << "\n\n";

    // ========================================
    // EXAMPLE 3: Monitoring (automatic)
    // ========================================

    std::cout << "--- Example 3: System Status ---\n";

    // Check metrics (automatically collected)
    auto metrics = system.get_metrics();
    std::cout << "Performance Metrics:\n";
    std::cout << "  Tasks submitted: " << metrics.tasks_submitted << "\n";
    std::cout << "  Tasks completed: " << metrics.tasks_completed << "\n";
    std::cout << "  Active workers: " << metrics.active_workers << "\n";
    std::cout << "  Queue size: " << metrics.queue_size << "\n";
    std::cout << "  Average latency: " << metrics.average_latency.count() << " ns\n\n";

    // Check health status (automatically monitored)
    auto health = system.get_health();
    std::cout << "Health Status: ";
    switch (health.overall_health) {
        case health_level::healthy:
            std::cout << "Healthy ✅\n";
            break;
        case health_level::degraded:
            std::cout << "Degraded ⚠️\n";
            break;
        case health_level::critical:
            std::cout << "Critical ❌\n";
            break;
        case health_level::failed:
            std::cout << "Failed 💥\n";
            break;
    }

    std::cout << "CPU Usage: " << health.cpu_usage_percent << "%\n";
    std::cout << "Memory Usage: " << health.memory_usage_percent << "%\n";
    std::cout << "Queue Utilization: " << health.queue_utilization_percent << "%\n";

    if (!health.issues.empty()) {
        std::cout << "Issues detected:\n";
        for (const auto& issue : health.issues) {
            std::cout << "  - " << issue << "\n";
        }
    }
    std::cout << "\n";

    // ========================================
    // EXAMPLE 4: Optional manual logging
    // ========================================

    std::cout << "--- Example 4: Manual Logging ---\n";

    // Manual logging if needed (automatic logging already handles most cases)
    system.log(log_level::info, "Manual log message from main thread");
    system.log(log_level::warning, "This is a warning message");

    // Submit a task that also logs
    auto logging_task = system.submit([&system]() -> int {
        system.log(log_level::debug, "Task is executing on worker thread");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        system.log(log_level::info, "Task completed successfully");
        return 999;
    });

    int result = logging_task.get();
    std::cout << "Logging task result: " << result << "\n\n";

    // ========================================
    // COMPARISON WITH ORIGINAL COMPLEXITY
    // ========================================

    std::cout << "--- Complexity Comparison ---\n";
    std::cout << "Original unified thread_system usage:\n";
    std::cout << "  1. unified_thread_system system;\n";
    std::cout << "  2. auto future = system.submit(task);\n";
    std::cout << "  3. auto result = future.get();\n\n";

    std::cout << "Current separate systems would require:\n";
    std::cout << "  1. Create logger, monitoring, thread_pool separately\n";
    std::cout << "  2. Register each in service_container\n";
    std::cout << "  3. Start each system individually\n";
    std::cout << "  4. Handle lifecycle management\n";
    std::cout << "  5. Manage error handling for each component\n\n";

    std::cout << "This unified API eliminates that complexity!\n\n";

    // ========================================
    // GRACEFUL SHUTDOWN
    // ========================================

    std::cout << "--- Graceful Shutdown ---\n";

    // Wait for any remaining tasks
    system.wait_for_completion();

    std::cout << "All tasks completed. System will shutdown automatically.\n";
    std::cout << "(Logging and monitoring data has been preserved)\n\n";

    std::cout << "=== Simple Usage Example Completed ===\n";

    // Destructor automatically handles cleanup
    return 0;
}

/*
 * Expected Output:
 *
 * Creating thread system (auto-configured)...
 * System created with 8 workers and automatic logging/monitoring
 *
 * --- Example 1: Simple Tasks ---
 * Task 1 result: 42
 * Task 2 result: 30
 * Task 3 result: Hello from worker thread!
 *
 * --- Example 2: Batch Processing ---
 * Processed 20 items in parallel
 * Sum of squares: 2870
 *
 * --- Example 3: System Status ---
 * Performance Metrics:
 *   Tasks submitted: 24
 *   Tasks completed: 24
 *   Active workers: 8
 *   Queue size: 0
 *   Average latency: 50000 ns
 *
 * Health Status: Healthy ✅
 * CPU Usage: 15.2%
 * Memory Usage: 8.1%
 * Queue Utilization: 0.0%
 *
 * --- Example 4: Manual Logging ---
 * Logging task result: 999
 *
 * --- Complexity Comparison ---
 * Original unified thread_system usage:
 *   1. unified_thread_system system;
 *   2. auto future = system.submit(task);
 *   3. auto result = future.get();
 *
 * Current separate systems would require:
 *   1. Create logger, monitoring, thread_pool separately
 *   2. Register each in service_container
 *   3. Start each system individually
 *   4. Handle lifecycle management
 *   5. Manage error handling for each component
 *
 * This unified API eliminates that complexity!
 *
 * --- Graceful Shutdown ---
 * All tasks completed. System will shutdown automatically.
 * (Logging and monitoring data has been preserved)
 *
 * === Simple Usage Example Completed ===
 */