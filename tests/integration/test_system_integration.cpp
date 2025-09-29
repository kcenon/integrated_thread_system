/**
 * @file test_system_integration.cpp
 * @brief Integration tests for system combinations
 */

#include <gtest/gtest.h>
#include <kcenon/integrated/unified_thread_system.h>
#include <chrono>
#include <vector>

using namespace integrated_thread_system;
using namespace std::chrono_literals;

class SystemIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test thread + logger integration
TEST_F(SystemIntegrationTest, ThreadLoggerIntegration) {
    unified_thread_system::config cfg;
    cfg.thread_count = 4;
    cfg.log_directory = "./test_logs";
    cfg.enable_file_logging = true;
    cfg.enable_console_logging = true;

    unified_thread_system system(cfg);

    // Submit task that logs
    auto future = system.submit([&system]() {
        system.log(log_level::info, "Task started");
        std::this_thread::sleep_for(10ms);
        system.log(log_level::info, "Task completed");
        return 42;
    });

    EXPECT_EQ(future.get(), 42);

    // Verify logging occurred (would need to check log file in real test)
}

// Test thread + monitor integration
TEST_F(SystemIntegrationTest, ThreadMonitorIntegration) {
    unified_thread_system::config cfg;
    cfg.thread_count = 4;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;
    cfg.enable_monitoring = true;

    unified_thread_system system(cfg);

    // Submit tasks and monitor performance
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(system.submit([&system]() {
            std::this_thread::sleep_for(5ms);
            // Task completed (metrics tracked automatically)
        }));
    }

    // Wait for all tasks
    for (auto& f : futures) {
        f.wait();
    }

    // Verify metrics are being tracked
    auto metrics = system.get_metrics();
    EXPECT_EQ(metrics.tasks_completed, 10);
    EXPECT_TRUE(system.is_healthy());
}

// Test logger + monitor integration
TEST_F(SystemIntegrationTest, LoggerMonitorIntegration) {
    unified_thread_system::config cfg;
    cfg.thread_count = 1; // Minimal threads for this test
    cfg.enable_file_logging = true;
    cfg.enable_monitoring = true;
    cfg.log_directory = "./test_logs";

    unified_thread_system system(cfg);

    // Log messages and verify system health
    for (int i = 0; i < 5; ++i) {
        system.log(log_level::info, "Test message " + std::to_string(i));
    }

    // Verify system is healthy and monitoring works
    EXPECT_TRUE(system.is_healthy());
    auto health = system.get_health();
    EXPECT_EQ(health.overall_health, health_level::healthy);
}

// Test all systems integration
TEST_F(SystemIntegrationTest, AllSystemsIntegration) {
    unified_thread_system::config cfg;
    cfg.thread_count = 4;
    cfg.enable_file_logging = true;
    cfg.enable_console_logging = true;
    cfg.enable_monitoring = true;
    cfg.log_directory = "./test_logs";

    unified_thread_system system(cfg);

    // Submit tasks that use all systems
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 20; ++i) {
        futures.push_back(system.submit([&system, i]() {
            auto start = std::chrono::steady_clock::now();

            system.log(log_level::debug, "Processing task " + std::to_string(i));

            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(10 + i));

            auto duration = std::chrono::steady_clock::now() - start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            system.log(log_level::info, "Task " + std::to_string(i) + " completed in " + std::to_string(ms) + "ms");

            return i * 2;
        }));
    }

    // Collect results
    int sum = 0;
    for (auto& f : futures) {
        sum += f.get();
    }

    EXPECT_EQ(sum, 20 * 19); // Sum of 0*2 + 2*2 + 4*2 + ... + 38*2

    // Verify system metrics are being tracked
    auto metrics = system.get_metrics();
    EXPECT_EQ(metrics.tasks_completed, 20);
    EXPECT_TRUE(system.is_healthy());

    // Check detailed health
    auto health = system.get_health();
    EXPECT_EQ(health.overall_health, health_level::healthy);
}

// Test system reconfiguration
TEST_F(SystemIntegrationTest, SystemReconfiguration) {
    // Start with minimal configuration
    config initial_cfg;
    initial_cfg.enable_thread_system(true)
               .enable_logger_system(false)
               .enable_monitoring_system(false)
               .set_worker_count(2);

    unified_thread_system system(initial_cfg);

    // Submit task with initial config
    auto f1 = system.submit([]() { return 1; });
    EXPECT_EQ(f1.get(), 1);

    // Reconfigure to enable all systems
    config new_cfg;
    new_cfg.enable_all_systems()
           .set_worker_count(4)
           .set_log_file("reconfig.log");

    system.reconfigure(new_cfg);

    // Now we can use logging and monitoring
    system.register_metric("test_metric", metric_type::counter);
    system.increment_counter("test_metric");
    system.log_info("Reconfiguration successful");

    EXPECT_EQ(system.get_counter("test_metric"), 1.0);
}

// Test selective system disabling
TEST_F(SystemIntegrationTest, SelectiveSystemDisabling) {
    // Test with only thread system
    {
        config cfg;
        cfg.enable_thread_system(true)
           .enable_logger_system(false)
           .enable_monitoring_system(false);

        unified_thread_system system(cfg);

        auto future = system.submit([]() { return true; });
        EXPECT_TRUE(future.get());

        // These should be no-ops (not crash)
        system.log_info("This won't be logged");
        system.increment_counter("non_existent");
    }

    // Test with only logger system
    {
        config cfg;
        cfg.enable_thread_system(false)
           .enable_logger_system(true)
           .enable_monitoring_system(false)
           .set_log_file("logger_only.log");

        unified_thread_system system(cfg);

        // Logging should work
        system.log_info("Logger-only mode");

        // Threading should throw or return error
        // (depending on implementation)
    }

    // Test with only monitor system
    {
        config cfg;
        cfg.enable_thread_system(false)
           .enable_logger_system(false)
           .enable_monitoring_system(true);

        unified_thread_system system(cfg);

        // Monitoring should work
        system.register_metric("test", metric_type::counter);
        system.increment_counter("test");
        EXPECT_EQ(system.get_counter("test"), 1.0);
    }
}

// Performance test for different configurations
TEST_F(SystemIntegrationTest, ConfigurationPerformanceComparison) {
    const int num_tasks = 1000;

    // Measure thread-only performance
    auto measure_performance = [num_tasks](const config& cfg) {
        unified_thread_system system(cfg);

        auto start = std::chrono::steady_clock::now();

        std::vector<std::future<int>> futures;
        for (int i = 0; i < num_tasks; ++i) {
            futures.push_back(system.submit([i]() {
                return i * 2;
            }));
        }

        for (auto& f : futures) {
            f.get();
        }

        auto duration = std::chrono::steady_clock::now() - start;
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    };

    // Thread-only
    config thread_only;
    thread_only.enable_thread_system(true)
               .enable_logger_system(false)
               .enable_monitoring_system(false);
    auto thread_only_ms = measure_performance(thread_only);

    // Thread + Logger
    config thread_logger;
    thread_logger.enable_thread_system(true)
                 .enable_logger_system(true)
                 .enable_monitoring_system(false)
                 .set_log_file("/dev/null");  // Discard logs
    auto thread_logger_ms = measure_performance(thread_logger);

    // All systems
    config all_systems;
    all_systems.enable_all_systems()
               .set_log_file("/dev/null");
    auto all_systems_ms = measure_performance(all_systems);

    std::cout << "Performance comparison for " << num_tasks << " tasks:" << std::endl;
    std::cout << "  Thread-only: " << thread_only_ms << "ms" << std::endl;
    std::cout << "  Thread+Logger: " << thread_logger_ms << "ms" << std::endl;
    std::cout << "  All systems: " << all_systems_ms << "ms" << std::endl;

    // Thread-only should be fastest
    EXPECT_LE(thread_only_ms, thread_logger_ms);
    EXPECT_LE(thread_only_ms, all_systems_ms);
}

// Test priority scheduling with monitoring
TEST_F(SystemIntegrationTest, PrioritySchedulingWithMonitoring) {
    config cfg;
    cfg.enable_thread_system(true)
       .enable_monitoring_system(true)
       .set_worker_count(2);  // Limited workers to test priority

    unified_thread_system system(cfg);

    // Register metrics for each priority
    system.register_metric("critical_tasks", metric_type::counter);
    system.register_metric("normal_tasks", metric_type::counter);
    system.register_metric("background_tasks", metric_type::counter);

    // Submit tasks with different priorities
    std::vector<std::future<void>> futures;

    // Background tasks (should run last)
    for (int i = 0; i < 5; ++i) {
        futures.push_back(system.submit_background([&system]() {
            system.increment_counter("background_tasks");
            std::this_thread::sleep_for(10ms);
        }));
    }

    // Normal tasks
    for (int i = 0; i < 5; ++i) {
        futures.push_back(system.submit([&system]() {
            system.increment_counter("normal_tasks");
            std::this_thread::sleep_for(10ms);
        }));
    }

    // Critical tasks (should run first)
    for (int i = 0; i < 5; ++i) {
        futures.push_back(system.submit_critical([&system]() {
            system.increment_counter("critical_tasks");
            std::this_thread::sleep_for(10ms);
        }));
    }

    // Wait for all tasks
    for (auto& f : futures) {
        f.wait();
    }

    // Verify all tasks completed
    EXPECT_EQ(system.get_counter("critical_tasks"), 5.0);
    EXPECT_EQ(system.get_counter("normal_tasks"), 5.0);
    EXPECT_EQ(system.get_counter("background_tasks"), 5.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}