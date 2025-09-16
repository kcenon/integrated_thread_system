/**
 * @file integrated_monitoring_example.cpp
 * @brief Example demonstrating integrated metrics and error handling
 */

#include <iostream>
#include <thread>
#include <random>
#include <kcenon/integrated/metrics_aggregator.h>
#include <kcenon/integrated/error_handler.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/adapters/thread_pool_executor.h>
#include <kcenon/monitoring/adapters/monitor_adapter.h>

using namespace kcenon::integrated;
using namespace kcenon::thread;
using namespace kcenon::monitoring;

/**
 * @brief Simulated worker that generates metrics and errors
 */
class simulated_worker {
public:
    simulated_worker(std::shared_ptr<thread_pool> pool,
                    error_handler& error_handler)
        : pool_(pool), error_handler_(error_handler) {
        
        // Initialize random generator
        std::random_device rd;
        gen_ = std::mt19937(rd());
        error_dist_ = std::uniform_real_distribution<>(0.0, 1.0);
        latency_dist_ = std::uniform_int_distribution<>(10, 200);
    }
    
    void perform_work(const std::string& task_name) {
        error_context ctx("SimulatedWorker", task_name);
        
        // Simulate work with random latency
        auto latency = std::chrono::milliseconds(latency_dist_(gen_));
        std::this_thread::sleep_for(latency);
        
        // Randomly generate errors
        double error_roll = error_dist_(gen_);
        
        if (error_roll < 0.05) { // 5% critical error rate
            ctx.add_metadata("error_type", "critical");
            error_handler_.report_error(
                error_severity::critical,
                "Critical error in " + task_name,
                ctx
            );
            throw std::runtime_error("Critical failure");
        }
        else if (error_roll < 0.15) { // 10% regular error rate
            ctx.add_metadata("error_type", "regular");
            error_handler_.report_error(
                error_severity::error,
                "Error processing " + task_name,
                ctx
            );
        }
        else if (error_roll < 0.25) { // 10% warning rate
            ctx.add_metadata("error_type", "warning");
            error_handler_.report_error(
                error_severity::warning,
                "Warning in " + task_name,
                ctx
            );
        }
        
        work_count_++;
    }
    
    std::size_t get_work_count() const {
        return work_count_.load();
    }
    
private:
    std::shared_ptr<thread_pool> pool_;
    error_handler& error_handler_;
    std::atomic<std::size_t> work_count_{0};
    
    std::mt19937 gen_;
    std::uniform_real_distribution<> error_dist_;
    std::uniform_int_distribution<> latency_dist_;
};

int main() {
    std::cout << "=== Integrated Monitoring Example ===" << std::endl;
    
    // Create event bus for system-wide events
    auto event_bus = std::make_shared<thread::event_bus>();
    
    // Initialize error handler
    std::cout << "\n1. Initializing error handler..." << std::endl;
    error_handler::config error_config;
    error_config.enable_recovery = true;
    error_config.min_severity = error_severity::info;
    
    error_handler err_handler(error_config, event_bus);
    err_handler.start();
    
    // Register error callback
    auto error_callback_id = err_handler.register_callback(
        error_severity::error,
        [](const error_info& error) {
            std::cout << "[ERROR CALLBACK] " << error.message 
                     << " from " << error.context.component << std::endl;
        }
    );
    
    // Initialize metrics aggregator
    std::cout << "\n2. Initializing metrics aggregator..." << std::endl;
    metrics_aggregator::config metrics_config;
    metrics_config.collection_interval = std::chrono::milliseconds(500);
    metrics_config.cpu_threshold = 70.0;
    metrics_config.error_rate_threshold = 10.0;
    
    metrics_aggregator aggregator(metrics_config, event_bus);
    
    // Subscribe to metrics alerts
    auto alert_sub = event_bus->subscribe<metrics_alert_event>(
        [](const metrics_alert_event& alert) {
            std::cout << "[METRICS ALERT] " << alert.message 
                     << " (current: " << alert.current_value 
                     << ", threshold: " << alert.threshold << ")" << std::endl;
        }
    );
    
    // Subscribe to error recovery events
    auto recovery_sub = event_bus->subscribe<error_recovered_event>(
        [](const error_recovered_event& event) {
            std::cout << "[RECOVERY] Component " << event.component 
                     << " recovered from error " << event.error_code 
                     << " using " << event.recovery_action << std::endl;
        }
    );
    
    // Create thread pool and monitoring components
    std::cout << "\n3. Setting up thread pool and monitoring..." << std::endl;
    auto thread_pool = std::make_shared<thread::thread_pool>(4);
    auto executor = std::make_shared<adapters::thread_pool_executor>(thread_pool);
    auto monitor = std::make_shared<adapters::monitor_adapter>();
    
    executor->initialize();
    monitor->initialize();
    
    // Register components with metrics aggregator
    aggregator.register_component("thread_executor", executor);
    aggregator.register_component("system_monitor", monitor);
    
    // Start metrics collection
    aggregator.start();
    
    // Create simulated worker
    std::cout << "\n4. Starting simulated workload..." << std::endl;
    simulated_worker worker(thread_pool, err_handler);
    
    // Submit work tasks
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 20; ++i) {
        futures.push_back(executor->execute([&worker, i]() {
            try {
                worker.perform_work("Task_" + std::to_string(i));
            } catch (const std::exception& e) {
                // Exception caught and reported
            }
        }));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Wait for some tasks to complete
    std::cout << "\n5. Processing tasks..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Display current metrics
    std::cout << "\n6. Current metrics:" << std::endl;
    auto current_metrics = aggregator.get_current_metrics();
    std::cout << "   Thread metrics:" << std::endl;
    std::cout << "     - Active threads: " << current_metrics.thread_metrics.active_threads << std::endl;
    std::cout << "     - Queued tasks: " << current_metrics.thread_metrics.queued_tasks << std::endl;
    std::cout << "   System metrics:" << std::endl;
    std::cout << "     - CPU usage: " << current_metrics.system_metrics.cpu_usage_percent << "%" << std::endl;
    std::cout << "     - Memory: " << current_metrics.system_metrics.memory_usage_mb << " MB" << std::endl;
    
    // Display error statistics
    std::cout << "\n7. Error statistics:" << std::endl;
    auto error_stats = err_handler.get_statistics();
    for (const auto& [severity, count] : error_stats) {
        std::cout << "   - Severity " << static_cast<int>(severity) << ": " << count << " errors" << std::endl;
    }
    
    // Display recent errors
    std::cout << "\n8. Recent errors:" << std::endl;
    auto recent_errors = err_handler.get_recent_errors(5);
    for (const auto& error : recent_errors) {
        std::cout << "   - [" << error.context.component << "] " 
                 << error.message << std::endl;
    }
    
    // Wait for all tasks
    std::cout << "\n9. Waiting for all tasks to complete..." << std::endl;
    for (auto& future : futures) {
        try {
            future.wait();
        } catch (...) {
            // Ignore exceptions
        }
    }
    
    // Calculate average metrics
    std::cout << "\n10. Average metrics over last 10 seconds:" << std::endl;
    auto avg_metrics = aggregator.calculate_average(std::chrono::seconds(10));
    std::cout << "   - Avg CPU: " << avg_metrics.system_metrics.cpu_usage_percent << "%" << std::endl;
    std::cout << "   - Avg Memory: " << avg_metrics.system_metrics.memory_usage_mb << " MB" << std::endl;
    std::cout << "   - Avg Threads: " << avg_metrics.thread_metrics.active_threads << std::endl;
    
    // Export metrics to JSON
    std::cout << "\n11. Exporting metrics to JSON..." << std::endl;
    std::string json_metrics = aggregator.export_to_json(current_metrics);
    std::cout << json_metrics.substr(0, 200) << "..." << std::endl; // Show first 200 chars
    
    // Cleanup
    std::cout << "\n12. Cleaning up..." << std::endl;
    aggregator.stop();
    err_handler.stop();
    executor->shutdown();
    monitor->shutdown();
    
    std::cout << "\nTotal work completed: " << worker.get_work_count() << " tasks" << std::endl;
    std::cout << "\nExample completed successfully!" << std::endl;
    
    return 0;
}
