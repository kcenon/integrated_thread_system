/**
 * @file test_minimal_improvements.cpp
 * @brief Minimal test showing basic improvements
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace kcenon::integrated;

int main() {
    std::cout << "\n=== Integrated Thread System Basic Test ===\n\n";

    // Create system with default configuration
    unified_thread_system system;

    // 1. Basic task submission
    std::cout << "1. Basic Task Submission:\n";
    auto future = system.submit([]() {
        std::cout << "   Task executing successfully\n";
        return 42;
    });
    int result = future.get();
    std::cout << "   Result: " << result << "\n\n";

    // 2. Multiple tasks
    std::cout << "2. Multiple Tasks:\n";
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(system.submit([i]() {
            std::cout << "   Task " << i << " running\n";
            return i * i;
        }));
    }

    int sum = 0;
    for (auto& f : futures) {
        sum += f.get();
    }
    std::cout << "   Sum of squares (0-4): " << sum << "\n\n";

    // 3. Batch processing
    std::cout << "3. Batch Processing:\n";
    std::vector<int> data = {10, 20, 30};
    auto batch_futures = system.submit_batch(data.begin(), data.end(),
        [](int x) {
            std::cout << "   Processing: " << x << "\n";
            return x * 2;
        });

    std::cout << "   Results: ";
    for (auto& f : batch_futures) {
        std::cout << f.get() << " ";
    }
    std::cout << "\n\n";

    // 4. Performance metrics
    std::cout << "4. Performance Metrics:\n";
    auto metrics = system.get_metrics();
    std::cout << "   Tasks submitted: " << metrics.tasks_submitted << "\n";
    std::cout << "   Tasks completed: " << metrics.tasks_completed << "\n";
    std::cout << "   Active workers: " << metrics.active_workers << "\n";
    std::cout << "   Queue size: " << metrics.queue_size << "\n\n";

    // 5. Health status
    std::cout << "5. System Health:\n";
    auto health = system.get_health();
    std::string health_str = "Unknown";
    switch (health.overall_health) {
        case health_level::healthy: health_str = "Healthy"; break;
        case health_level::degraded: health_str = "Degraded"; break;
        case health_level::critical: health_str = "Critical"; break;
        case health_level::failed: health_str = "Failed"; break;
    }
    std::cout << "   Overall health: " << health_str << "\n";
    std::cout << "   CPU usage: " << health.cpu_usage_percent << "%\n";
    std::cout << "   Memory usage: " << health.memory_usage_percent << "%\n\n";

    // 6. Wait for completion
    std::cout << "6. Waiting for all tasks to complete...\n";
    system.wait_for_completion();
    std::cout << "   All tasks completed!\n\n";

    std::cout << "=== Test Completed Successfully ===\n\n";

    std::cout << "Key Improvements Demonstrated:\n";
    std::cout << "• Zero-configuration thread pool setup\n";
    std::cout << "• Automatic performance metrics collection\n";
    std::cout << "• Health monitoring integration\n";
    std::cout << "• Batch processing support\n";
    std::cout << "• Clean shutdown handling\n\n";

    return 0;
}