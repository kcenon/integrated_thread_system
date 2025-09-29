/**
 * @file test_enhanced.cpp
 * @brief Test program for enhanced unified thread system
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <vector>
#include <chrono>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== Testing Enhanced Unified Thread System ===\n\n";

    // Create thread system with enhanced configuration
    config cfg;
    cfg.name = "TestEnhanced";
    cfg.thread_count = 4;
    cfg.enable_circuit_breaker = true;
    cfg.circuit_breaker_failure_threshold = 3;
    cfg.enable_work_stealing = true;
    cfg.enable_console_logging = true;
    cfg.min_log_level = log_level::info;

    try {
        unified_thread_system system(cfg);
        std::cout << "✓ System created with " << system.worker_count() << " workers\n";

        // Test 1: Basic task submission
        std::cout << "\n1. Basic Task Submission:\n";
        auto future1 = system.submit([]() -> int {
            std::this_thread::sleep_for(50ms);
            return 42;
        });
        std::cout << "   Result: " << future1.get() << "\n";

        // Test 2: Priority task submission
        std::cout << "\n2. Priority Task Submission:\n";
        std::vector<std::future<std::string>> priority_futures;

        // Submit tasks with different priorities
        priority_futures.push_back(system.submit_background([](){
            std::this_thread::sleep_for(10ms);
            return std::string("Background task completed");
        }));

        priority_futures.push_back(system.submit([](){
            std::this_thread::sleep_for(10ms);
            return std::string("Normal task completed");
        }));

        priority_futures.push_back(system.submit_critical([](){
            std::this_thread::sleep_for(10ms);
            return std::string("Critical task completed");
        }));

        // Collect results
        for (auto& f : priority_futures) {
            std::cout << "   " << f.get() << "\n";
        }

        // Test 3: Batch processing
        std::cout << "\n3. Batch Processing:\n";
        std::vector<int> data = {1, 2, 3, 4, 5};
        auto batch_futures = system.submit_batch(
            data.begin(), data.end(),
            [](int n) -> int { return n * n; }
        );

        std::cout << "   Results: ";
        for (auto& f : batch_futures) {
            std::cout << f.get() << " ";
        }
        std::cout << "\n";

        // Test 4: Cancellation
        std::cout << "\n4. Cancellation Test:\n";
        cancellation_token cancel_token;
        auto cancellable = system.submit_cancellable(cancel_token, [](){
            std::this_thread::sleep_for(100ms);
            return std::string("Should be cancelled");
        });

        // Cancel immediately
        cancel_token.cancel();
        std::this_thread::sleep_for(10ms); // Give it time to process

        try {
            auto result = cancellable.get();
            std::cout << "   Result: " << (result.empty() ? "Cancelled (empty)" : result) << "\n";
        } catch (const std::exception& e) {
            std::cout << "   Cancelled with exception: " << e.what() << "\n";
        }

        // Test 5: Scheduled task
        std::cout << "\n5. Scheduled Task:\n";
        auto scheduled = system.schedule(100ms, [](){
            return std::string("Delayed task executed");
        });

        auto start = std::chrono::steady_clock::now();
        std::cout << "   " << scheduled.get();
        auto duration = std::chrono::steady_clock::now() - start;
        std::cout << " (after "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << "ms)\n";

        // Test 6: Performance metrics
        std::cout << "\n6. Performance Metrics:\n";
        auto metrics = system.get_metrics();
        std::cout << "   Tasks submitted: " << metrics.tasks_submitted << "\n";
        std::cout << "   Tasks completed: " << metrics.tasks_completed << "\n";
        std::cout << "   Tasks failed: " << metrics.tasks_failed << "\n";
        std::cout << "   Average latency: " << metrics.average_latency.count() << "ns\n";

        // Test 7: Health status
        std::cout << "\n7. Health Status:\n";
        auto health = system.get_health();
        std::cout << "   Overall health: ";
        switch (health.overall_health) {
            case health_level::healthy: std::cout << "Healthy ✅\n"; break;
            case health_level::degraded: std::cout << "Degraded ⚠️\n"; break;
            case health_level::critical: std::cout << "Critical ❌\n"; break;
            case health_level::failed: std::cout << "Failed 💥\n"; break;
        }
        std::cout << "   Circuit breaker: "
                  << (health.circuit_breaker_open ? "OPEN" : "CLOSED") << "\n";

        // Test 8: Map-Reduce
        std::cout << "\n8. Map-Reduce Test:\n";
        std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto map_reduce_future = system.map_reduce(
            numbers.begin(), numbers.end(),
            [](int n) -> int { return n * n; },  // Map: square each number
            [](int a, int b) -> int { return a + b; },  // Reduce: sum
            0  // Initial value
        );
        std::cout << "   Sum of squares (1-10): " << map_reduce_future.get() << "\n";

        // Test 9: Circuit breaker (intentional failures)
        std::cout << "\n9. Circuit Breaker Test:\n";
        if (cfg.enable_circuit_breaker) {
            // Submit tasks that will fail
            for (int i = 0; i < 4; ++i) {
                auto fail_future = system.submit([i]() -> int {
                    throw std::runtime_error("Intentional failure " + std::to_string(i));
                });

                try {
                    fail_future.get();
                } catch (const std::exception& e) {
                    std::cout << "   Task " << i << " failed: " << e.what() << "\n";
                }
            }

            // Check if circuit breaker opened
            if (system.is_circuit_open()) {
                std::cout << "   ✓ Circuit breaker OPENED after failures\n";

                // Reset it
                system.reset_circuit_breaker();
                std::cout << "   ✓ Circuit breaker RESET\n";
            }
        }

        // Final wait
        system.wait_for_completion();
        std::cout << "\n=== All tests completed successfully! ===\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}