/**
 * @file 03_corrected_basic_example.cpp
 * @brief Corrected version of basic examples with proper includes and improvements
 */

#include <kcenon/integrated/unified_thread_system.h>  // Correct include path
#include <iostream>
#include <vector>
#include <numeric>
#include <chrono>
#include <thread>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

// Helper function for clean output
void print_example(int number, const std::string& title) {
    std::cout << "\n" << number << ". " << title << ":\n";
    std::cout << std::string(40, '-') << "\n";
}

int main() {
    std::cout << "=== Corrected Basic Examples with Improvements ===\n";

    // Use the unified system with proper configuration
    unified_thread_system::config config;
    config.name = "BasicExamples";
    config.thread_count = 0;  // Auto-detect
    config.enable_console_logging = true;
    config.min_log_level = log_level::info;

    unified_thread_system system(config);

    // Example 1: Simple task with return value (CORRECTED)
    print_example(1, "Task with return value");
    {
        auto future = system.submit([]() {
            int sum = 0;
            for (int i = 1; i <= 100; ++i) {
                sum += i;
            }
            return sum;
        });

        std::cout << "Sum of 1 to 100 = " << future.get() << "\n";
        system.log(log_level::info, "Calculation completed successfully");
    }

    // Example 2: Task with parameters using lambda capture (IMPROVED)
    print_example(2, "Task with parameters and monitoring");
    {
        int multiplier = 5;
        int value = 10;

        auto future = system.submit([multiplier, value]() {
            // Simulate some work
            std::this_thread::sleep_for(10ms);
            return multiplier * value;
        });

        std::cout << multiplier << " * " << value << " = " << future.get() << "\n";

        // Show metrics (improvement from monitoring_system)
        auto metrics = system.get_metrics();
        std::cout << "Tasks completed so far: " << metrics.tasks_completed << "\n";
    }

    // Example 3: Batch processing (IMPROVED with timing)
    print_example(3, "Batch processing with performance measurement");
    {
        std::vector<int> data = {1, 2, 3, 4, 5};

        auto start = std::chrono::steady_clock::now();

        // Use batch processing from improved API
        auto futures = system.submit_batch(data.begin(), data.end(),
            [](int x) {
                std::this_thread::sleep_for(20ms);  // Simulate work
                return x * x;
            });

        std::cout << "Squares: ";
        for (auto& future : futures) {
            std::cout << future.get() << " ";
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "\nTime taken: " << duration.count() << "ms";
        std::cout << " (vs " << (data.size() * 20) << "ms if sequential)\n";
    }

    // Example 4: Error handling (IMPROVED)
    print_example(4, "Robust error handling");
    {
        auto future = system.submit([]() -> int {
            static int call_count = 0;
            call_count++;
            if (call_count == 1) {
                throw std::runtime_error("First call always fails (testing)");
            }
            return 42;
        });

        try {
            auto result = future.get();
            std::cout << "Unexpected success: " << result << "\n";
        } catch (const std::exception& e) {
            std::cout << "Handled error gracefully: " << e.what() << "\n";
            system.log(log_level::warning, std::string("Task failed: ") + e.what());
        }

        // Retry
        auto retry_future = system.submit([]() { return 42; });
        std::cout << "Retry succeeded: " << retry_future.get() << "\n";
    }

    // Example 5: Health monitoring (NEW from monitoring_system)
    print_example(5, "System health monitoring");
    {
        auto health = system.get_health();

        std::cout << "System health: ";
        switch (health.overall_health) {
            case health_level::healthy:
                std::cout << "Healthy ✓\n";
                break;
            case health_level::degraded:
                std::cout << "Degraded ⚠\n";
                break;
            case health_level::critical:
                std::cout << "Critical ✗\n";
                break;
            case health_level::failed:
                std::cout << "Failed ✗\n";
                break;
        }

        std::cout << "CPU usage: " << health.cpu_usage_percent << "%\n";
        std::cout << "Memory usage: " << health.memory_usage_percent << "%\n";
        std::cout << "Queue utilization: " << health.queue_utilization_percent << "%\n";
    }

    // Example 6: Parallel data processing (IMPROVED)
    print_example(6, "Parallel data processing with metrics");
    {
        std::vector<int> large_data(20);
        std::iota(large_data.begin(), large_data.end(), 1);  // Fill with 1,2,3...20

        auto futures = system.submit_batch(
            large_data.begin(), large_data.end(),
            [](int x) {
                // Simulate complex processing
                std::this_thread::sleep_for(5ms);
                double result = std::sqrt(x * x + 1.0);
                return static_cast<int>(result * 100);
            });

        int total = 0;
        for (auto& f : futures) {
            total += f.get();
        }

        std::cout << "Processed " << large_data.size() << " items\n";
        std::cout << "Total result: " << total << "\n";

        // Show final metrics
        auto metrics = system.get_metrics();
        std::cout << "Total tasks submitted: " << metrics.tasks_submitted << "\n";
        std::cout << "Total tasks completed: " << metrics.tasks_completed << "\n";
        std::cout << "Total tasks failed: " << metrics.tasks_failed << "\n";
    }

    // Example 7: Logging at different levels (from logger_system)
    print_example(7, "Multi-level logging integration");
    {
        system.log(log_level::trace, "Trace message - very detailed");
        system.log(log_level::debug, "Debug message - for debugging");
        system.log(log_level::info, "Info message - normal operation");
        system.log(log_level::warning, "Warning message - potential issue");
        system.log(log_level::error, "Error message - recoverable error");
        system.log(log_level::critical, "Critical message - serious issue");

        std::cout << "Logging at multiple levels completed\n";
        std::cout << "(Check console output based on min_log_level setting)\n";
    }

    // Summary
    std::cout << "\n=== Summary of Improvements Applied ===\n";
    std::cout << "✓ Correct include paths (<kcenon/integrated/...>)\n";
    std::cout << "✓ Proper configuration with config struct\n";
    std::cout << "✓ Batch processing with submit_batch()\n";
    std::cout << "✓ Performance metrics from monitoring_system\n";
    std::cout << "✓ Health monitoring integration\n";
    std::cout << "✓ Multi-level logging from logger_system\n";
    std::cout << "✓ Error handling and recovery\n";
    std::cout << "✓ Parallel execution with timing measurements\n";

    std::cout << "\n=== All examples completed successfully! ===\n";

    return 0;
}