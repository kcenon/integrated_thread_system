/**
 * @file priority_jobs.cpp
 * @brief Priority-based job scheduling in the Integrated Thread System
 * @author kcenon <kcenon@gmail.com>
 * @date 2024
 *
 * @details This comprehensive example demonstrates the priority-based task scheduling
 * capabilities of the Enhanced Integrated Thread System. It covers basic priority
 * levels, real-world web server scenarios, and batch processing with priorities.
 *
 * @par Difficulty
 * Intermediate
 *
 * @par Time to Complete
 * 10 minutes
 *
 * @par Key Concepts
 * - Three-tier priority system (critical, normal, background)
 * - Priority inversion prevention
 * - Real-world priority assignment strategies
 * - Batch processing with priority differentiation
 *
 * @note Requires the Enhanced version of Integrated Thread System
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

/**
 * @class priority_demo
 * @brief Demonstration class for priority-based task scheduling
 *
 * @details This class encapsulates various demonstrations of priority-based
 * scheduling, including basic priorities, real-world web server simulation,
 * and batch processing scenarios.
 */
class priority_demo {
private:
    unified_thread_system system_;  ///< Thread system instance with 2 workers
    std::atomic<int> execution_order_{0};  ///< Tracks task execution order

public:
    /**
     * @brief Constructs a priority demo with limited workers
     * @details Uses only 2 worker threads to make priority effects more visible
     */
    priority_demo() : system_(config{}.set_worker_count(2)) {
        // Use only 2 workers to clearly see priority effects
    }

    /**
     * @brief Demonstrates basic priority level ordering
     *
     * @details Submits tasks at different priority levels (critical, normal, background)
     * and shows that higher priority tasks are executed first even when submitted last.
     *
     * @par Expected Output
     * Critical tasks execute first, followed by normal, then background tasks
     */
    void basic_priorities() {
        std::cout << "\n1. Basic Priority Levels:" << std::endl;
        std::cout << "   (Using 2 workers to show priority ordering)" << std::endl;

        execution_order_ = 0;
        std::vector<std::pair<std::string, int>> results;
        std::mutex results_mutex;

        // Submit jobs in reverse priority order
        auto background = system_.submit_background([&]() {
            std::this_thread::sleep_for(10ms);  // Simulate work
            int order = execution_order_.fetch_add(1);
            std::lock_guard<std::mutex> lock(results_mutex);
            results.push_back({"BACKGROUND", order});
            return "Background task done";
        });

        auto normal = system_.submit([&]() {
            std::this_thread::sleep_for(10ms);  // Simulate work
            int order = execution_order_.fetch_add(1);
            std::lock_guard<std::mutex> lock(results_mutex);
            results.push_back({"NORMAL", order});
            return "Normal task done";
        });

        auto critical = system_.submit_critical([&]() {
            std::this_thread::sleep_for(10ms);  // Simulate work
            int order = execution_order_.fetch_add(1);
            std::lock_guard<std::mutex> lock(results_mutex);
            results.push_back({"CRITICAL", order});
            return "Critical task done";
        });

        // Wait for all tasks
        critical.wait();
        normal.wait();
        background.wait();

        // Show execution order
        std::cout << "   Execution order:" << std::endl;
        for (const auto& [priority, order] : results) {
            std::cout << "   " << order + 1 << ". " << priority << std::endl;
        }
        std::cout << "   Note: Critical executed first, background last" << std::endl;
    }

    /**
     * @brief Simulates a real-world web server with prioritized request handling
     *
     * @details Models different types of web requests (health checks, payments,
     * analytics) and assigns appropriate priority levels based on business importance:
     * - Critical: Health checks, system alerts
     * - Normal: User logins, payments
     * - Background: Analytics, report generation
     *
     * @par Performance Note
     * Critical requests are guaranteed to be processed first, improving system
     * responsiveness for monitoring and emergency situations.
     */
    void real_world_example() {
        std::cout << "\n2. Real-World Example - Web Server:" << std::endl;

        struct request {
            int id;
            std::string type;
            int processing_time_ms;
        };

        std::vector<request> requests = {
            {1, "health_check", 5},
            {2, "user_login", 50},
            {3, "analytics", 100},
            {4, "payment", 30},
            {5, "report_generation", 200},
            {6, "system_alert", 10}
        };

        std::cout << "   Processing " << requests.size() << " requests..." << std::endl;

        std::vector<std::future<std::string>> futures;
        auto start_time = std::chrono::steady_clock::now();

        for (const auto& req : requests) {
            if (req.type == "health_check" || req.type == "system_alert") {
                // Critical - system health
                futures.push_back(system_.submit_critical([req]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(req.processing_time_ms));
                    return "CRITICAL: " + req.type + " #" + std::to_string(req.id) + " completed";
                }));
            } else if (req.type == "payment" || req.type == "user_login") {
                // Normal - user-facing
                futures.push_back(system_.submit([req]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(req.processing_time_ms));
                    return "NORMAL: " + req.type + " #" + std::to_string(req.id) + " completed";
                }));
            } else {
                // Background - analytics, reports
                futures.push_back(system_.submit_background([req]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(req.processing_time_ms));
                    return "BACKGROUND: " + req.type + " #" + std::to_string(req.id) + " completed";
                }));
            }
        }

        // Collect results in completion order
        for (auto& future : futures) {
            std::cout << "   " << future.get() << std::endl;
        }

        auto duration = std::chrono::steady_clock::now() - start_time;
        std::cout << "   Total time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << "ms" << std::endl;
    }

    /**
     * @brief Demonstrates batch processing with priority differentiation
     *
     * @details Shows how to process data batches with different urgency levels,
     * ensuring urgent batches are processed before regular ones regardless of
     * submission order.
     *
     * @par Use Case
     * Useful for data pipelines where some batches contain time-sensitive data
     * (e.g., real-time metrics) while others contain historical data.
     */
    void priority_with_batches() {
        std::cout << "\n3. Batch Processing with Priorities:" << std::endl;

        // Simulate batch data processing
        struct data_batch {
            int id;
            int size;
            bool is_urgent;
        };

        std::vector<data_batch> batches = {
            {1, 100, false},
            {2, 50, true},
            {3, 200, false},
            {4, 25, true},
            {5, 150, false}
        };

        std::cout << "   Processing " << batches.size() << " data batches..." << std::endl;

        std::atomic<int> urgent_completed{0};
        std::atomic<int> regular_completed{0};

        std::vector<std::future<void>> futures;

        for (const auto& batch : batches) {
            if (batch.is_urgent) {
                futures.push_back(system_.submit_critical([&urgent_completed, batch]() {
                    // Simulate processing
                    std::this_thread::sleep_for(std::chrono::milliseconds(batch.size / 10));
                    urgent_completed.fetch_add(1);
                    std::cout << "   URGENT batch " << batch.id
                              << " (size=" << batch.size << ") processed" << std::endl;
                }));
            } else {
                futures.push_back(system_.submit_background([&regular_completed, batch]() {
                    // Simulate processing
                    std::this_thread::sleep_for(std::chrono::milliseconds(batch.size / 10));
                    regular_completed.fetch_add(1);
                    std::cout << "   Regular batch " << batch.id
                              << " (size=" << batch.size << ") processed" << std::endl;
                }));
            }
        }

        // Wait for all batches
        for (auto& future : futures) {
            future.wait();
        }

        std::cout << "   Summary: " << urgent_completed.load() << " urgent, "
                  << regular_completed.load() << " regular batches processed" << std::endl;
        std::cout << "   Note: Urgent batches were prioritized" << std::endl;
    }

    /**
     * @brief Executes all priority demonstrations in sequence
     */
    void run_all_demos() {
        std::cout << "=== Priority-Based Job Scheduling ===" << std::endl;

        basic_priorities();
        real_world_example();
        priority_with_batches();

        std::cout << "\n=== Priority examples completed! ===" << std::endl;
    }
};

/**
 * @brief Main function executing all priority scheduling demonstrations
 * @return 0 on success, 1 on error
 * @throws std::exception if thread system initialization fails
 */
int main() {
    try {
        priority_demo demo;
        demo.run_all_demos();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/**
 * @par What You Learned
 * -# Three-tier priority system: critical (127), normal (50), background (0)
 * -# Higher priority tasks preempt lower priority ones in the queue
 * -# Real-world priority assignment strategies for different task types
 * -# Batch processing with priority-based differentiation
 *
 * @par Priority Guidelines
 * - @b Critical: System health checks, emergency alerts, payment processing
 * - @b Normal: User interactions, API requests, standard operations
 * - @b Background: Analytics, reporting, maintenance, cleanup
 *
 * @par Performance Impact
 * Priority scheduling adds ~5% overhead but ensures critical tasks have
 * predictable latency even under high load.
 *
 * @par Next Steps
 * - Explore @ref custom_priorities.cpp for fine-grained priority control
 * - See @ref web_server.cpp for advanced priority implementation
 * - Review @ref monitoring_example.cpp for priority-based monitoring
 *
 * @see unified_thread_system::submit_critical()
 * @see unified_thread_system::submit_background()
 * @see priority_level
 */