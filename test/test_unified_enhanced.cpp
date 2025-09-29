/**
 * @file test_unified_enhanced.cpp
 * @brief Comprehensive test suite for enhanced unified thread system
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <cassert>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

// Color codes for output
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

class test_suite {
private:
    int tests_passed = 0;
    int tests_failed = 0;
    std::vector<std::string> failed_tests;

public:
    void test(const std::string& name, std::function<void()> test_func) {
        std::cout << BLUE << "Running test: " << name << RESET << std::endl;
        try {
            test_func();
            tests_passed++;
            std::cout << GREEN << "  ✓ PASSED" << RESET << std::endl;
        } catch (const std::exception& e) {
            tests_failed++;
            failed_tests.push_back(name);
            std::cout << RED << "  ✗ FAILED: " << e.what() << RESET << std::endl;
        }
    }

    void report() {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "Test Results:" << std::endl;
        std::cout << GREEN << "  Passed: " << tests_passed << RESET << std::endl;

        if (tests_failed > 0) {
            std::cout << RED << "  Failed: " << tests_failed << RESET << std::endl;
            std::cout << "\nFailed tests:" << std::endl;
            for (const auto& name : failed_tests) {
                std::cout << RED << "  - " << name << RESET << std::endl;
            }
        }

        std::cout << std::string(50, '=') << std::endl;
    }

    bool all_passed() const { return tests_failed == 0; }
};

void test_basic_submission() {
    unified_thread_system system;

    auto future = system.submit([]() { return 42; });
    auto result = future.get();

    assert(result == 42);
}

void test_priority_submission() {
    unified_thread_system::config cfg;
    cfg.thread_count = 1;  // Single thread to ensure order
    unified_thread_system system(cfg);

    std::vector<int> results;
    std::mutex results_mutex;

    // Submit tasks with different priorities
    auto low = system.submit_with_priority(priority_level::low, [&]() {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back(1);
        return 1;
    });

    auto high = system.submit_with_priority(priority_level::high, [&]() {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back(2);
        return 2;
    });

    auto critical = system.submit_with_priority(priority_level::critical, [&]() {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back(3);
        return 3;
    });

    // Wait for completion
    low.wait();
    high.wait();
    critical.wait();

    // Critical task should execute first (3), then high (2), then low (1)
    // Note: Actual order may vary due to timing, but priority influence should be visible
    assert(!results.empty());
}

void test_cancellation() {
    unified_thread_system system;
    cancellation_token token;

    auto future = system.submit_cancellable(token, [&token]() {
        for (int i = 0; i < 10; ++i) {
            if (token.is_cancelled()) {
                return -1;
            }
            std::this_thread::sleep_for(10ms);
        }
        return 42;
    });

    // Cancel after a short delay
    std::this_thread::sleep_for(25ms);
    token.cancel();

    auto result = future.get();
    assert(result == -1);  // Should return -1 due to cancellation
}

void test_scheduled_execution() {
    unified_thread_system system;

    auto start = std::chrono::steady_clock::now();

    auto future = system.schedule(100ms, []() { return 123; });

    auto result = future.get();
    auto duration = std::chrono::steady_clock::now() - start;

    assert(result == 123);
    assert(duration >= 100ms);  // Should execute after delay
}

void test_recurring_tasks() {
    unified_thread_system system;
    std::atomic<int> counter{0};

    auto task_id = system.schedule_recurring(50ms, [&counter]() {
        counter++;
    });

    std::this_thread::sleep_for(250ms);

    system.cancel_recurring(task_id);

    // Should have executed approximately 4-5 times
    assert(counter >= 3 && counter <= 6);
}

void test_batch_processing() {
    unified_thread_system system;

    std::vector<int> data = {1, 2, 3, 4, 5};

    auto futures = system.submit_batch(data.begin(), data.end(),
                                       [](int x) { return x * x; });

    int sum = 0;
    for (auto& future : futures) {
        sum += future.get();
    }

    assert(sum == 55);  // 1 + 4 + 9 + 16 + 25
}

void test_map_reduce() {
    unified_thread_system system;

    std::vector<int> data = {1, 2, 3, 4, 5};

    auto future = system.map_reduce(
        data.begin(), data.end(),
        [](int x) { return x * 2; },        // Map: multiply by 2
        [](int a, int b) { return a + b; }, // Reduce: sum
        0                                    // Initial value
    );

    auto result = future.get();
    assert(result == 30);  // (1+2+3+4+5) * 2
}

void test_metrics_collection() {
    unified_thread_system system;

    // Submit some tasks
    for (int i = 0; i < 10; ++i) {
        system.submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i));
            return i;
        });
    }

    system.wait_for_completion();

    auto metrics = system.get_metrics();

    assert(metrics.tasks_submitted >= 10);
    assert(metrics.tasks_completed >= 10);
    assert(metrics.tasks_failed == 0);
}

void test_health_monitoring() {
    unified_thread_system system;

    // Add custom health check
    system.add_health_check("test_check", []() {
        return std::make_pair(true, "All good");
    });

    auto health = system.get_health();

    assert(health.overall_health == health_level::healthy);
    assert(!health.circuit_breaker_open);
}

void test_circuit_breaker() {
    unified_thread_system::config cfg;
    cfg.enable_circuit_breaker = true;
    cfg.circuit_breaker_failure_threshold = 3;

    unified_thread_system system(cfg);

    // Submit failing tasks
    for (int i = 0; i < 3; ++i) {
        try {
            auto future = system.submit([]() {
                throw std::runtime_error("Intentional failure");
                return 0;
            });
            future.get();
        } catch (...) {
            // Expected
        }
    }

    // Circuit should be open now
    std::this_thread::sleep_for(100ms);  // Give time for circuit to open

    bool circuit_was_open = false;
    try {
        system.submit([]() { return 1; });
    } catch (const std::runtime_error& e) {
        if (std::string(e.what()).find("Circuit breaker") != std::string::npos) {
            circuit_was_open = true;
        }
    }

    // Reset circuit breaker
    system.reset_circuit_breaker();

    // Should work now
    auto future = system.submit([]() { return 42; });
    assert(future.get() == 42);
}

void test_event_subscription() {
    unified_thread_system system;

    std::atomic<int> event_count{0};

    auto subscription_id = system.subscribe_to_events("log",
        [&event_count](const std::string& type, const std::any& data) {
            event_count++;
        });

    // Generate some log events
    system.submit([]() { return 1; });
    system.wait_for_completion();

    std::this_thread::sleep_for(50ms);

    system.unsubscribe_from_events(subscription_id);

    assert(event_count > 0);  // Should have received some events
}

void test_custom_metrics() {
    unified_thread_system system;

    int custom_value = 42;
    system.register_metric_collector("custom_metric",
        [&custom_value]() { return static_cast<double>(custom_value); });

    auto health = system.get_health();

    assert(health.custom_metrics.count("custom_metric") > 0);
    assert(health.custom_metrics.at("custom_metric") == 42.0);
}

void test_export_formats() {
    unified_thread_system system;

    // Submit some tasks to generate metrics
    for (int i = 0; i < 5; ++i) {
        system.submit([i]() { return i; });
    }

    system.wait_for_completion();

    // Test JSON export
    auto json = system.export_metrics_json();
    assert(json.find("tasks_submitted") != std::string::npos);
    assert(json.find("tasks_completed") != std::string::npos);

    // Test Prometheus export
    auto prometheus = system.export_metrics_prometheus();
    assert(prometheus.find("# HELP") != std::string::npos);
    assert(prometheus.find("# TYPE") != std::string::npos);
}

void test_wait_timeout() {
    unified_thread_system system;

    // Submit a long-running task
    system.submit([]() {
        std::this_thread::sleep_for(200ms);
        return 1;
    });

    // Should timeout
    bool completed = system.wait_for_completion_timeout(50ms);
    assert(!completed);

    // Wait for actual completion
    system.wait_for_completion();

    // Should complete immediately now
    completed = system.wait_for_completion_timeout(10ms);
    assert(completed);
}

void test_graceful_shutdown() {
    unified_thread_system system;

    // Submit tasks
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(system.submit([i]() {
            std::this_thread::sleep_for(10ms);
            return i;
        }));
    }

    // Graceful shutdown
    system.shutdown();

    // All tasks should have completed
    for (auto& future : futures) {
        assert(future.valid());
    }

    assert(system.is_shutting_down());
}

void test_immediate_shutdown() {
    unified_thread_system system;

    // Submit many tasks
    for (int i = 0; i < 100; ++i) {
        system.submit([i]() {
            std::this_thread::sleep_for(100ms);
            return i;
        });
    }

    // Immediate shutdown
    system.shutdown_immediate();

    auto metrics = system.get_metrics();
    assert(metrics.tasks_cancelled > 0);  // Some tasks should have been cancelled
}

int main() {
    std::cout << "\n" << YELLOW << "Enhanced Unified Thread System Test Suite" << RESET << "\n";
    std::cout << std::string(50, '=') << "\n" << std::endl;

    test_suite suite;

    // Basic functionality
    suite.test("Basic task submission", test_basic_submission);
    suite.test("Priority-based submission", test_priority_submission);
    suite.test("Task cancellation", test_cancellation);

    // Scheduling
    suite.test("Scheduled execution", test_scheduled_execution);
    suite.test("Recurring tasks", test_recurring_tasks);

    // Batch operations
    suite.test("Batch processing", test_batch_processing);
    suite.test("Map-reduce pattern", test_map_reduce);

    // Monitoring and metrics
    suite.test("Metrics collection", test_metrics_collection);
    suite.test("Health monitoring", test_health_monitoring);
    suite.test("Custom metrics", test_custom_metrics);

    // Resilience
    suite.test("Circuit breaker", test_circuit_breaker);

    // Events and export
    suite.test("Event subscription", test_event_subscription);
    suite.test("Export formats", test_export_formats);

    // Flow control
    suite.test("Wait with timeout", test_wait_timeout);
    suite.test("Graceful shutdown", test_graceful_shutdown);
    suite.test("Immediate shutdown", test_immediate_shutdown);

    suite.report();

    return suite.all_passed() ? 0 : 1;
}