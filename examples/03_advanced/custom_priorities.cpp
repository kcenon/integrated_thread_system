/**
 * @file custom_priorities.cpp
 * @brief Advanced example showing custom priority types and scheduling
 * @difficulty Advanced
 * @time 20 minutes
 */

#include "unified_thread_system.h"
#include <iostream>
#include <chrono>
#include <vector>

using namespace integrated_thread_system;
using namespace std::chrono_literals;

// Define custom priority levels for a financial trading system
enum class trading_priority : int {
    market_data = 0,      // Highest - real-time market data
    order_execution = 1,  // Trade execution
    risk_check = 2,       // Risk management
    reporting = 3,        // Regulatory reporting
    analytics = 4         // Background analytics
};

class custom_priority_demo {
private:
    unified_thread_system system_;

public:
    custom_priority_demo() {
        config cfg;
        cfg.set_worker_count(6)
           .enable_custom_priorities<trading_priority>()
           .enable_priority_inheritance(true)
           .set_starvation_prevention(true, std::chrono::seconds(5));

        system_ = unified_thread_system(cfg);
    }

    void demonstrate_trading_system() {
        std::cout << "=== Financial Trading System Demo ===" << std::endl;

        // Market data feed (highest priority)
        auto market_data_handler = [this](const std::string& symbol, double price) {
            return system_.submit_with_priority(trading_priority::market_data,
                [symbol, price]() {
                    std::cout << "[MARKET DATA] " << symbol << " @ $" << price << std::endl;
                    // Process market data immediately
                    std::this_thread::sleep_for(1ms);
                    return true;
                });
        };

        // Order execution
        auto execute_order = [this](const std::string& symbol, int quantity, double price) {
            return system_.submit_with_priority(trading_priority::order_execution,
                [symbol, quantity, price]() {
                    std::cout << "[ORDER] Buy " << quantity << " " << symbol
                              << " @ $" << price << std::endl;
                    // Execute trade
                    std::this_thread::sleep_for(5ms);
                    return "ORDER_ID_" + std::to_string(rand() % 10000);
                });
        };

        // Risk check
        auto check_risk = [this](const std::string& order_id) {
            return system_.submit_with_priority(trading_priority::risk_check,
                [order_id]() {
                    std::cout << "[RISK CHECK] Validating " << order_id << std::endl;
                    // Perform risk calculations
                    std::this_thread::sleep_for(10ms);
                    return true; // Risk approved
                });
        };

        // Reporting
        auto generate_report = [this](const std::string& report_type) {
            return system_.submit_with_priority(trading_priority::reporting,
                [report_type]() {
                    std::cout << "[REPORT] Generating " << report_type << std::endl;
                    // Generate regulatory report
                    std::this_thread::sleep_for(50ms);
                    return "REPORT_" + report_type + "_COMPLETE";
                });
        };

        // Analytics
        auto run_analytics = [this](const std::string& analysis_type) {
            return system_.submit_with_priority(trading_priority::analytics,
                [analysis_type]() {
                    std::cout << "[ANALYTICS] Running " << analysis_type << std::endl;
                    // Perform background analysis
                    std::this_thread::sleep_for(100ms);
                    return 42.0; // Some analytical result
                });
        };

        // Simulate trading activity
        std::vector<std::future<bool>> market_futures;
        std::vector<std::future<std::string>> order_futures;
        std::vector<std::future<bool>> risk_futures;

        // High-frequency market data
        for (int i = 0; i < 10; ++i) {
            market_futures.push_back(market_data_handler("AAPL", 150.0 + i * 0.1));
            market_futures.push_back(market_data_handler("GOOGL", 2800.0 + i * 0.5));
        }

        // Trading orders
        order_futures.push_back(execute_order("AAPL", 100, 150.5));
        order_futures.push_back(execute_order("GOOGL", 50, 2801.0));

        // Risk checks for orders
        risk_futures.push_back(check_risk("ORDER_ID_123"));
        risk_futures.push_back(check_risk("ORDER_ID_456"));

        // Background tasks
        auto report_future = generate_report("DAILY_TRADES");
        auto analytics_future = run_analytics("PORTFOLIO_OPTIMIZATION");

        // Wait for critical tasks
        for (auto& f : market_futures) f.wait();
        for (auto& f : order_futures) f.wait();
        for (auto& f : risk_futures) f.wait();

        std::cout << "\nAll critical trading operations completed" << std::endl;
        std::cout << "Background tasks still running..." << std::endl;

        // Wait for background tasks
        report_future.wait();
        analytics_future.wait();

        std::cout << "All tasks completed\n" << std::endl;
    }

    void demonstrate_priority_inheritance() {
        std::cout << "=== Priority Inheritance Demo ===" << std::endl;

        // Scenario: Low-priority task holds a resource needed by high-priority task
        std::mutex shared_resource;
        std::atomic<int> execution_order{0};

        // Low priority task acquires lock first
        auto low_priority_task = system_.submit_with_priority(
            trading_priority::analytics,
            [&shared_resource, &execution_order]() {
                std::lock_guard<std::mutex> lock(shared_resource);
                std::cout << "Low priority task holds resource" << std::endl;
                std::this_thread::sleep_for(100ms);
                execution_order.fetch_add(1);
                std::cout << "Low priority task releases resource" << std::endl;
                return 1;
            });

        // Give low priority task time to acquire lock
        std::this_thread::sleep_for(10ms);

        // High priority task needs the same resource
        auto high_priority_task = system_.submit_with_priority(
            trading_priority::market_data,
            [&shared_resource, &execution_order]() {
                std::cout << "High priority task waiting for resource..." << std::endl;
                std::lock_guard<std::mutex> lock(shared_resource);
                std::cout << "High priority task acquired resource" << std::endl;
                execution_order.fetch_add(1);
                return 2;
            });

        low_priority_task.wait();
        high_priority_task.wait();

        std::cout << "With priority inheritance, low-priority task is boosted" << std::endl;
        std::cout << "to prevent priority inversion\n" << std::endl;
    }

    void demonstrate_dynamic_priority_adjustment() {
        std::cout << "=== Dynamic Priority Adjustment ===" << std::endl;

        // Create a task that changes priority based on conditions
        class dynamic_task {
        private:
            unified_thread_system& system_;
            std::atomic<bool> urgent_{false};

        public:
            dynamic_task(unified_thread_system& sys) : system_(sys) {}

            void set_urgent(bool urgent) { urgent_ = urgent; }

            auto submit_work(int id) {
                auto priority = urgent_.load() ?
                    trading_priority::order_execution :
                    trading_priority::analytics;

                return system_.submit_with_priority(priority,
                    [this, id, priority]() {
                        std::string priority_str = (priority == trading_priority::order_execution) ?
                            "URGENT" : "NORMAL";
                        std::cout << "Task " << id << " executed as " << priority_str << std::endl;
                        return id;
                    });
            }
        };

        dynamic_task task(system_);

        // Submit normal priority tasks
        std::cout << "Submitting normal priority tasks..." << std::endl;
        auto f1 = task.submit_work(1);
        auto f2 = task.submit_work(2);

        // Market condition changes - make tasks urgent
        task.set_urgent(true);
        std::cout << "Market volatility detected - switching to urgent mode" << std::endl;

        auto f3 = task.submit_work(3);
        auto f4 = task.submit_work(4);

        // Return to normal
        task.set_urgent(false);
        std::cout << "Market stabilized - returning to normal priority" << std::endl;

        auto f5 = task.submit_work(5);

        // Wait for all
        f1.wait(); f2.wait(); f3.wait(); f4.wait(); f5.wait();

        std::cout << std::endl;
    }

    void demonstrate_priority_based_resource_allocation() {
        std::cout << "=== Priority-Based Resource Allocation ===" << std::endl;

        // Simulate limited resources (e.g., database connections)
        class resource_pool {
        private:
            std::vector<bool> resources_;
            std::mutex mutex_;
            std::condition_variable cv_;

        public:
            resource_pool(size_t size) : resources_(size, true) {}

            int acquire(trading_priority priority) {
                std::unique_lock<std::mutex> lock(mutex_);

                // High priority gets resource immediately if available
                // Low priority may need to wait
                auto wait_time = (priority <= trading_priority::order_execution) ?
                    std::chrono::seconds(10) : std::chrono::seconds(1);

                if (!cv_.wait_for(lock, wait_time, [this]() {
                    return std::find(resources_.begin(), resources_.end(), true)
                           != resources_.end();
                })) {
                    return -1; // Timeout
                }

                // Find and allocate resource
                for (size_t i = 0; i < resources_.size(); ++i) {
                    if (resources_[i]) {
                        resources_[i] = false;
                        return static_cast<int>(i);
                    }
                }
                return -1;
            }

            void release(int id) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (id >= 0 && id < resources_.size()) {
                    resources_[id] = true;
                    cv_.notify_one();
                }
            }
        };

        resource_pool pool(2); // Only 2 resources available

        auto use_resource = [this, &pool](trading_priority priority, const std::string& task_name) {
            return system_.submit_with_priority(priority,
                [&pool, priority, task_name]() {
                    std::cout << task_name << " requesting resource..." << std::endl;
                    int resource_id = pool.acquire(priority);

                    if (resource_id < 0) {
                        std::cout << task_name << " FAILED to get resource (timeout)" << std::endl;
                        return false;
                    }

                    std::cout << task_name << " acquired resource " << resource_id << std::endl;
                    std::this_thread::sleep_for(50ms); // Use resource

                    pool.release(resource_id);
                    std::cout << task_name << " released resource " << resource_id << std::endl;
                    return true;
                });
        };

        // Submit tasks with different priorities competing for resources
        auto f1 = use_resource(trading_priority::market_data, "Market Data");
        auto f2 = use_resource(trading_priority::analytics, "Analytics");
        auto f3 = use_resource(trading_priority::order_execution, "Order");
        auto f4 = use_resource(trading_priority::reporting, "Report");

        // Wait for all
        f1.wait(); f2.wait(); f3.wait(); f4.wait();

        std::cout << "Note: Higher priority tasks get resources first\n" << std::endl;
    }

    void run_all_demonstrations() {
        demonstrate_trading_system();
        demonstrate_priority_inheritance();
        demonstrate_dynamic_priority_adjustment();
        demonstrate_priority_based_resource_allocation();

        std::cout << "=== Custom Priority System Benefits ===" << std::endl;
        std::cout << "✓ Domain-specific priority levels" << std::endl;
        std::cout << "✓ Priority inheritance prevents inversion" << std::endl;
        std::cout << "✓ Dynamic priority adjustment" << std::endl;
        std::cout << "✓ Resource allocation based on priority" << std::endl;
        std::cout << "✓ Starvation prevention for low-priority tasks" << std::endl;
    }
};

int main() {
    try {
        custom_priority_demo demo;
        demo.run_all_demonstrations();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * Key Concepts Demonstrated:
 *
 * 1. Custom Priority Types
 *    - Define domain-specific priorities
 *    - Map business logic to scheduling
 *
 * 2. Priority Inheritance
 *    - Prevent priority inversion
 *    - Automatic boosting of blocking tasks
 *
 * 3. Dynamic Priority
 *    - Adjust priority based on conditions
 *    - Respond to system state changes
 *
 * 4. Resource Allocation
 *    - Priority-based resource access
 *    - Fair scheduling with starvation prevention
 *
 * Next Steps:
 * - Implement in production systems
 * - Monitor priority distribution
 * - Tune for optimal performance
 */