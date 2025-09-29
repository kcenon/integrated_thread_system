/**
 * @file priority_jobs.cpp
 * @brief Learn how to use priority-based job scheduling
 * @difficulty Intermediate
 * @time 10 minutes
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

class priority_demo {
private:
    unified_thread_system system_;
    std::atomic<int> execution_order_{0};

public:
    priority_demo() : system_(config{}.set_worker_count(2)) {
        // Use only 2 workers to clearly see priority effects
    }

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

    void run_all_demos() {
        std::cout << "=== Priority-Based Job Scheduling ===" << std::endl;

        basic_priorities();
        real_world_example();
        priority_with_batches();

        std::cout << "\n=== Priority examples completed! ===" << std::endl;
    }
};

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

/*
 * What you learned:
 * 1. Three priority levels: critical, normal, background
 * 2. Higher priority jobs are processed first
 * 3. Real-world use cases for different priorities
 * 4. Batch processing with priority assignment
 *
 * Key concepts:
 * - Critical: System health, emergencies
 * - Normal: User-facing operations
 * - Background: Analytics, maintenance
 *
 * Next steps:
 * - Try batch_processing.cpp for advanced batch operations
 * - Try error_recovery.cpp for error handling with priorities
 */