/**
 * @file dependency_injection_example.cpp
 * @brief Example demonstrating dependency injection with service registry
 */

#include <iostream>
#include <kcenon/thread/interfaces/shared_interfaces.h>
#include <kcenon/thread/core/service_registry.h>
#include <kcenon/thread/adapters/thread_pool_executor.h>
#include <kcenon/logger/adapters/logger_adapter.h>
#include <kcenon/monitoring/adapters/monitor_adapter.h>

using namespace kcenon;

/**
 * @brief Example service that uses injected dependencies
 */
class application_service : public shared::IService {
public:
    application_service(thread::service_registry& registry)
        : registry_(registry) {
    }

    bool initialize() override {
        std::cout << "Initializing application service..." << std::endl;

        // Get required services from registry
        executor_ = registry_.get_service<shared::IExecutor>();
        logger_ = registry_.get_service<shared::ILogger>();
        monitor_ = registry_.get_service<shared::IMonitorable>();

        if (!executor_) {
            std::cerr << "Failed to get executor service!" << std::endl;
            return false;
        }

        if (logger_) {
            logger_->log(shared::LogLevel::Info, "Application service initialized");
        }

        is_running_ = true;
        return true;
    }

    void shutdown() override {
        std::cout << "Shutting down application service..." << std::endl;

        if (logger_) {
            logger_->log(shared::LogLevel::Info, "Application service shutting down");
        }

        is_running_ = false;
    }

    bool is_running() const override {
        return is_running_;
    }

    std::string name() const override {
        return "ApplicationService";
    }

    /**
     * @brief Run a sample task using injected services
     */
    void run_task() {
        if (!executor_) {
            std::cerr << "No executor available!" << std::endl;
            return;
        }

        std::cout << "Submitting task to executor..." << std::endl;

        auto future = executor_->execute([this]() {
            if (logger_) {
                logger_->log(shared::LogLevel::Info, "Task started");
            }

            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (logger_) {
                logger_->log(shared::LogLevel::Info, "Task completed");
            }
        });

        // Wait for task completion
        future.wait();

        // Get metrics if monitor is available
        if (monitor_) {
            auto metrics = monitor_->get_metrics();
            std::cout << "Current metrics:" << std::endl;
            std::cout << "  CPU Usage: " << metrics.cpu_usage << "%" << std::endl;
            std::cout << "  Memory: " << metrics.memory_usage_mb << " MB" << std::endl;
            std::cout << "  Active Threads: " << metrics.active_threads << std::endl;
        }
    }

private:
    thread::service_registry& registry_;
    std::shared_ptr<shared::IExecutor> executor_;
    std::shared_ptr<shared::ILogger> logger_;
    std::shared_ptr<shared::IMonitorable> monitor_;
    bool is_running_{false};
};

int main() {
    std::cout << "=== Dependency Injection Example ===" << std::endl;

    // Get the service registry
    auto& registry = thread::service_registry::instance();

    // Register services
    std::cout << "\n1. Registering services..." << std::endl;

    // Register thread pool executor
    auto executor = std::make_shared<thread::adapters::thread_pool_executor>(4);
    registry.register_service<shared::IExecutor>(executor);
    std::cout << "   - Executor registered" << std::endl;

    // Register logger
    auto logger = std::make_shared<logger::adapters::logger_adapter>();
    registry.register_service<shared::ILogger>(logger);
    std::cout << "   - Logger registered" << std::endl;

    // Register monitor
    auto monitor = std::make_shared<monitoring::adapters::monitor_adapter>();
    registry.register_service<shared::IMonitorable>(monitor);
    std::cout << "   - Monitor registered" << std::endl;

    // Initialize all services
    std::cout << "\n2. Initializing services..." << std::endl;
    executor->initialize();
    logger->initialize();
    monitor->initialize();

    // Create and use application service
    std::cout << "\n3. Creating application service..." << std::endl;
    application_service app_service(registry);

    if (app_service.initialize()) {
        std::cout << "\n4. Running application task..." << std::endl;
        app_service.run_task();

        // Run another task
        std::cout << "\n5. Running another task..." << std::endl;
        app_service.run_task();
    }

    // Cleanup
    std::cout << "\n6. Cleaning up..." << std::endl;
    app_service.shutdown();
    monitor->shutdown();
    logger->shutdown();
    executor->shutdown();

    std::cout << "\nExample completed successfully!" << std::endl;

    return 0;
}