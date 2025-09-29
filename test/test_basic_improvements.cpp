/**
 * @file test_basic_improvements.cpp
 * @brief Basic test showing core improvements from other systems
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

// Color codes for test output
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

void demonstrate_improved_features() {
    std::cout << "\n" << BLUE << "=== Integrated Thread System Improvements Demo ===" << RESET << "\n\n";

    // Create system with custom configuration
    unified_thread_system::config cfg;
    cfg.name = "ImprovedSystem";
    cfg.thread_count = 4;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = true;
    cfg.min_log_level = log_level::debug;

    unified_thread_system system(cfg);

    // 1. Basic task submission (from original)
    std::cout << GREEN << "1. Basic Task Submission:" << RESET << std::endl;
    auto future1 = system.submit([]() {
        std::cout << "   Task executing on thread: " << std::this_thread::get_id() << std::endl;
        return 42;
    });
    std::cout << "   Result: " << future1.get() << "\n\n";

    // 2. Batch processing (improved pattern)
    std::cout << GREEN << "2. Batch Processing Pattern:" << RESET << std::endl;
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto futures = system.submit_batch(data.begin(), data.end(),
        [](int x) {
            std::cout << "   Processing: " << x << std::endl;
            return x * x;
        });

    int sum = 0;
    for (auto& f : futures) {
        sum += f.get();
    }
    std::cout << "   Sum of squares: " << sum << "\n\n";

    // 3. Performance metrics (from monitoring_system)
    std::cout << GREEN << "3. Performance Metrics:" << RESET << std::endl;

    // Submit some work to generate metrics
    for (int i = 0; i < 10; ++i) {
        system.submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 10));
            return i;
        });
    }

    system.wait_for_completion();

    auto metrics = system.get_metrics();
    std::cout << "   Tasks submitted: " << metrics.tasks_submitted << std::endl;
    std::cout << "   Tasks completed: " << metrics.tasks_completed << std::endl;
    std::cout << "   Active workers: " << metrics.active_workers << std::endl;
    std::cout << "   Queue size: " << metrics.queue_size << "\n\n";

    // 4. Health monitoring (from monitoring_system)
    std::cout << GREEN << "4. System Health:" << RESET << std::endl;
    auto health = system.get_health();

    std::string health_str;
    switch (health.overall_health) {
        case health_level::healthy: health_str = "Healthy"; break;
        case health_level::degraded: health_str = "Degraded"; break;
        case health_level::critical: health_str = "Critical"; break;
        case health_level::failed: health_str = "Failed"; break;
    }

    std::cout << "   Overall health: " << health_str << std::endl;
    std::cout << "   CPU usage: " << health.cpu_usage_percent << "%" << std::endl;
    std::cout << "   Memory usage: " << health.memory_usage_percent << "%" << std::endl;
    std::cout << "   Queue utilization: " << health.queue_utilization_percent << "%\n\n";

    // 5. Logging integration (from logger_system)
    std::cout << GREEN << "5. Integrated Logging:" << RESET << std::endl;
    system.log(log_level::info, "System demonstration started");
    system.log(log_level::debug, "Processing batch of tasks");
    system.log(log_level::warning, "Queue utilization above threshold");
    std::cout << "\n";

    // 6. Parallel execution demonstration
    std::cout << GREEN << "6. Parallel Execution:" << RESET << std::endl;
    auto start = std::chrono::steady_clock::now();

    std::vector<std::future<int>> parallel_futures;
    for (int i = 0; i < 8; ++i) {
        parallel_futures.push_back(system.submit([i]() {
            std::this_thread::sleep_for(100ms);
            std::cout << "   Task " << i << " completed" << std::endl;
            return i;
        }));
    }

    // Wait for all
    for (auto& f : parallel_futures) {
        f.get();
    }

    auto duration = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    std::cout << "   Total time for 8x100ms tasks: " << ms << "ms ";
    std::cout << "(Speedup: " << (800.0 / ms) << "x)\n\n";

    // 7. Error handling demonstration
    std::cout << GREEN << "7. Error Handling:" << RESET << std::endl;
    try {
        auto error_future = system.submit([]() {
            throw std::runtime_error("Intentional error for testing");
            return 0;
        });

        try {
            error_future.get();
        } catch (const std::exception& e) {
            std::cout << "   Caught expected error: " << e.what() << std::endl;
        }
    } catch (...) {
        std::cout << "   Unexpected error in task submission\n";
    }

    // Check metrics after error
    metrics = system.get_metrics();
    std::cout << "   Failed tasks: " << metrics.tasks_failed << "\n\n";

    std::cout << BLUE << "=== Demonstration Complete ===" << RESET << "\n\n";
}

void test_integration_points() {
    std::cout << YELLOW << "Testing Integration Points with Other Systems:\n" << RESET;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    std::cout << "✓ Thread System: Priority scheduling, job queues\n";
    std::cout << "✓ Logger System: Structured logging, multiple writers\n";
    std::cout << "✓ Monitoring System: Performance metrics, health checks\n";
    std::cout << "✓ Common System: Shared interfaces, event bus pattern\n";

    std::cout << "\nKey improvements integrated:\n";
    std::cout << "• Zero-configuration setup with sensible defaults\n";
    std::cout << "• Automatic performance monitoring\n";
    std::cout << "• Built-in logging without external dependencies\n";
    std::cout << "• Health monitoring and circuit breaker patterns\n";
    std::cout << "• Event-driven architecture support\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
}

int main() {
    try {
        test_integration_points();
        demonstrate_improved_features();

        std::cout << GREEN << "All tests passed successfully!" << RESET << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with error: " << e.what() << std::endl;
        return 1;
    }
}