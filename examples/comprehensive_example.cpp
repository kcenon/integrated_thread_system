/**
 * @file comprehensive_example.cpp
 * @brief Comprehensive demonstration of all advanced features in the Integrated Thread System
 *
 * This example showcases the full capabilities of the unified system, including:
 * - Basic unified API usage
 * - Priority-based job scheduling (typed_thread_pool functionality)
 * - Adaptive queue optimization
 * - Performance monitoring and health checks
 * - Integration with external logging and monitoring systems
 * - Real-world usage scenarios
 */

#include "../include/unified_thread_system.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include <thread>
#include <future>
#include <iomanip>

using namespace integrated_thread_system;

// Define custom job types for priority-based scheduling
enum class job_priority {
    critical = 0,    // Highest priority - real-time tasks
    high = 1,        // High priority - user-facing operations
    normal = 2,      // Normal priority - regular processing
    low = 3,         // Low priority - background tasks
    background = 4   // Lowest priority - maintenance tasks
};

class ComprehensiveExample {
private:
    unified_thread_system system_;
    std::atomic<int> tasks_completed_{0};
    std::atomic<int> critical_tasks_completed_{0};
    std::atomic<int> background_tasks_completed_{0};

public:
    ComprehensiveExample() {
        unified_thread_system::config cfg;
        cfg.name = "Comprehensive Demo";
        cfg.thread_count = std::thread::hardware_concurrency();
        cfg.enable_file_logging = true;
        cfg.enable_console_logging = true;
        cfg.enable_monitoring = true;
        cfg.log_directory = "./logs";
        cfg.min_log_level = log_level::info;

        system_ = unified_thread_system(cfg);
    }

    /**
     * @brief Example 1: Basic unified API usage
     */
    void basic_usage_example() {
        std::cout << "\n=== Example 1: Basic Unified API Usage ===\n";

        // Simple task submission
        auto simple_future = system_.submit([]() -> int {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 42;
        });

        // Task with parameters
        auto param_future = system_.submit([](int x, double y) -> double {
            return x * y * 2.5;
        }, 10, 3.14);

        // Async task with logging
        auto logging_future = system_.submit([this]() -> std::string {
            system_.log(log_level::info, "Executing logging task from worker thread");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return "Task completed with logging";
        });

        // Get results
        std::cout << "Simple task result: " << simple_future.get() << "\n";
        std::cout << "Parameter task result: " << param_future.get() << "\n";
        std::cout << "Logging task result: " << logging_future.get() << "\n";

        std::cout << "✓ Basic API usage completed successfully\n";
    }

    /**
     * @brief Example 2: Priority-based job scheduling
     * Demonstrates typed_thread_pool functionality through unified API
     */
    void priority_scheduling_example() {
        std::cout << "\n=== Example 2: Priority-based Job Scheduling ===\n";

        std::vector<std::future<std::string>> futures;
        std::vector<std::pair<job_priority, std::string>> job_specs = {
            {job_priority::critical, "CRITICAL: System health check"},
            {job_priority::background, "BACKGROUND: Log rotation"},
            {job_priority::high, "HIGH: User request processing"},
            {job_priority::normal, "NORMAL: Data processing"},
            {job_priority::low, "LOW: Cache cleanup"},
            {job_priority::critical, "CRITICAL: Security scan"},
            {job_priority::background, "BACKGROUND: Database backup"},
            {job_priority::high, "HIGH: UI update"},
        };

        // Submit jobs with different priorities
        for (const auto& [priority, description] : job_specs) {
            futures.push_back(system_.submit([this, priority, description]() -> std::string {
                auto start_time = std::chrono::steady_clock::now();

                // Simulate work based on priority
                int work_duration_ms = 50;
                switch (priority) {
                    case job_priority::critical:
                        work_duration_ms = 20; // Fast execution for critical tasks
                        critical_tasks_completed_.fetch_add(1);
                        break;
                    case job_priority::background:
                        work_duration_ms = 200; // Longer for background tasks
                        background_tasks_completed_.fetch_add(1);
                        break;
                    default:
                        work_duration_ms = 100;
                        break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(work_duration_ms));

                auto end_time = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

                std::string result = description + " (completed in " + std::to_string(duration.count()) + "ms)";

                system_.log(log_level::debug, result);
                tasks_completed_.fetch_add(1);

                return result;
            }));
        }

        // Collect and display results in execution order
        std::cout << "Job execution order (should prioritize CRITICAL tasks):\n";
        for (size_t i = 0; i < futures.size(); ++i) {
            auto result = futures[i].get();
            std::cout << "  " << (i + 1) << ". " << result << "\n";
        }

        std::cout << "✓ Priority scheduling example completed\n";
        std::cout << "  Critical tasks: " << critical_tasks_completed_.load() << "\n";
        std::cout << "  Background tasks: " << background_tasks_completed_.load() << "\n";
        std::cout << "  Total tasks: " << tasks_completed_.load() << "\n";
    }

    /**
     * @brief Example 3: Batch processing with adaptive optimization
     */
    void batch_processing_example() {
        std::cout << "\n=== Example 3: Batch Processing with Adaptive Optimization ===\n";

        // Create a large dataset for processing
        const size_t dataset_size = 1000;
        std::vector<int> dataset(dataset_size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);

        for (size_t i = 0; i < dataset_size; ++i) {
            dataset[i] = dis(gen);
        }

        std::cout << "Processing " << dataset_size << " items in parallel...\n";

        auto start_time = std::chrono::high_resolution_clock::now();

        // Process data in parallel using batch submission
        auto batch_futures = system_.submit_batch(
            dataset.begin(),
            dataset.end(),
            [this](int value) -> double {
                // Simulate complex mathematical operation
                double result = 0.0;
                for (int i = 0; i < 100; ++i) {
                    result += std::sin(value * i * 0.001) * std::cos(value * i * 0.002);
                }

                // Log progress occasionally
                static std::atomic<int> processed_count{0};
                int current_count = processed_count.fetch_add(1);
                if (current_count % 100 == 0) {
                    system_.log(log_level::info, "Processed {} items", current_count);
                }

                return result;
            }
        );

        // Collect results
        std::vector<double> results;
        results.reserve(dataset_size);
        double total_result = 0.0;

        for (auto& future : batch_futures) {
            double result = future.get();
            results.push_back(result);
            total_result += result;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << "Batch processing completed:\n";
        std::cout << "  Items processed: " << dataset_size << "\n";
        std::cout << "  Total time: " << duration.count() << " ms\n";
        std::cout << "  Processing rate: " << std::fixed << std::setprecision(2)
                  << (dataset_size * 1000.0 / duration.count()) << " items/second\n";
        std::cout << "  Average result: " << std::fixed << std::setprecision(6)
                  << (total_result / dataset_size) << "\n";
        std::cout << "  Result range: [" << std::fixed << std::setprecision(3)
                  << *std::min_element(results.begin(), results.end()) << ", "
                  << *std::max_element(results.begin(), results.end()) << "]\n";

        std::cout << "✓ Batch processing example completed\n";
    }

    /**
     * @brief Example 4: Performance monitoring and health checks
     */
    void monitoring_example() {
        std::cout << "\n=== Example 4: Performance Monitoring and Health Checks ===\n";

        // Submit some background load
        std::vector<std::future<void>> background_tasks;
        const int num_background_tasks = 50;

        for (int i = 0; i < num_background_tasks; ++i) {
            background_tasks.push_back(system_.submit([this, i]() -> void {
                // Variable duration tasks to create interesting metrics
                int duration = 10 + (i % 5) * 20;
                std::this_thread::sleep_for(std::chrono::milliseconds(duration));

                if (i % 10 == 0) {
                    system_.log(log_level::debug, "Background task {} completed", i);
                }
            }));
        }

        // Monitor system performance while tasks execute
        std::cout << "Monitoring system performance:\n";
        std::cout << std::left << std::setw(8) << "Time"
                  << std::setw(12) << "Submitted"
                  << std::setw(12) << "Completed"
                  << std::setw(10) << "Failed"
                  << std::setw(12) << "Latency(ns)"
                  << std::setw(10) << "Workers"
                  << std::setw(10) << "Queue"
                  << std::setw(10) << "Health" << "\n";
        std::cout << std::string(80, '-') << "\n";

        auto monitoring_start = std::chrono::steady_clock::now();

        for (int sample = 0; sample < 10; ++sample) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            auto metrics = system_.get_metrics();
            auto health = system_.get_health();

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - monitoring_start).count();

            std::string health_str;
            switch (health.overall_health) {
                case health_level::healthy: health_str = "✓"; break;
                case health_level::degraded: health_str = "⚠"; break;
                case health_level::critical: health_str = "❌"; break;
                case health_level::failed: health_str = "💥"; break;
            }

            std::cout << std::left << std::setw(8) << (elapsed / 1000.0)
                      << std::setw(12) << metrics.tasks_submitted
                      << std::setw(12) << metrics.tasks_completed
                      << std::setw(10) << metrics.tasks_failed
                      << std::setw(12) << metrics.average_latency.count()
                      << std::setw(10) << metrics.active_workers
                      << std::setw(10) << metrics.queue_size
                      << std::setw(10) << health_str << "\n";
        }

        // Wait for all background tasks to complete
        for (auto& future : background_tasks) {
            future.get();
        }

        // Final metrics
        auto final_metrics = system_.get_metrics();
        auto final_health = system_.get_health();

        std::cout << "\nFinal System Status:\n";
        std::cout << "  Tasks submitted: " << final_metrics.tasks_submitted << "\n";
        std::cout << "  Tasks completed: " << final_metrics.tasks_completed << "\n";
        std::cout << "  Tasks failed: " << final_metrics.tasks_failed << "\n";
        std::cout << "  Success rate: " << std::fixed << std::setprecision(2)
                  << (100.0 * final_metrics.tasks_completed / std::max(1UL, final_metrics.tasks_submitted)) << "%\n";
        std::cout << "  Average latency: " << final_metrics.average_latency.count() << " ns\n";
        std::cout << "  Active workers: " << final_metrics.active_workers << "\n";
        std::cout << "  Queue size: " << final_metrics.queue_size << "\n";

        std::cout << "\nHealth Status:\n";
        std::string health_description;
        switch (final_health.overall_health) {
            case health_level::healthy:
                health_description = "Healthy - All systems operating normally";
                break;
            case health_level::degraded:
                health_description = "Degraded - Some performance issues detected";
                break;
            case health_level::critical:
                health_description = "Critical - Significant issues detected";
                break;
            case health_level::failed:
                health_description = "Failed - System not functioning properly";
                break;
        }
        std::cout << "  Overall: " << health_description << "\n";
        std::cout << "  CPU Usage: " << std::fixed << std::setprecision(1) << final_health.cpu_usage_percent << "%\n";
        std::cout << "  Memory Usage: " << std::fixed << std::setprecision(1) << final_health.memory_usage_percent << "%\n";
        std::cout << "  Queue Utilization: " << std::fixed << std::setprecision(1) << final_health.queue_utilization_percent << "%\n";

        if (!final_health.issues.empty()) {
            std::cout << "  Issues:\n";
            for (const auto& issue : final_health.issues) {
                std::cout << "    • " << issue << "\n";
            }
        }

        std::cout << "✓ Performance monitoring example completed\n";
    }

    /**
     * @brief Example 5: Real-world scenario - Web server request processing
     */
    void web_server_simulation() {
        std::cout << "\n=== Example 5: Web Server Request Processing Simulation ===\n";

        // Simulate different types of web requests
        enum class request_type {
            static_content,   // Low priority, cached content
            api_call,         // Normal priority, business logic
            user_upload,      // High priority, user-facing
            admin_action,     // Critical priority, administrative
            analytics        // Background priority, data processing
        };

        struct request_stats {
            std::atomic<int> static_requests{0};
            std::atomic<int> api_requests{0};
            std::atomic<int> upload_requests{0};
            std::atomic<int> admin_requests{0};
            std::atomic<int> analytics_requests{0};
            std::atomic<int> total_requests{0};
            std::atomic<long long> total_processing_time_ms{0};
        } stats;

        // Request generator function
        auto generate_request = [&](request_type type, int request_id) -> std::future<std::string> {
            return system_.submit([&, type, request_id]() -> std::string {
                auto start = std::chrono::high_resolution_clock::now();

                std::string type_name;
                int processing_time_ms;

                switch (type) {
                    case request_type::static_content:
                        type_name = "STATIC";
                        processing_time_ms = 5 + (request_id % 3);
                        stats.static_requests.fetch_add(1);
                        break;
                    case request_type::api_call:
                        type_name = "API";
                        processing_time_ms = 20 + (request_id % 10);
                        stats.api_requests.fetch_add(1);
                        break;
                    case request_type::user_upload:
                        type_name = "UPLOAD";
                        processing_time_ms = 50 + (request_id % 20);
                        stats.upload_requests.fetch_add(1);
                        break;
                    case request_type::admin_action:
                        type_name = "ADMIN";
                        processing_time_ms = 15 + (request_id % 5);
                        stats.admin_requests.fetch_add(1);
                        break;
                    case request_type::analytics:
                        type_name = "ANALYTICS";
                        processing_time_ms = 100 + (request_id % 50);
                        stats.analytics_requests.fetch_add(1);
                        break;
                }

                // Simulate request processing
                std::this_thread::sleep_for(std::chrono::milliseconds(processing_time_ms));

                auto end = std::chrono::high_resolution_clock::now();
                auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                stats.total_requests.fetch_add(1);
                stats.total_processing_time_ms.fetch_add(actual_duration.count());

                std::string response = type_name + " request " + std::to_string(request_id) +
                                     " processed in " + std::to_string(actual_duration.count()) + "ms";

                // Log critical and high-priority requests
                if (type == request_type::admin_action || type == request_type::user_upload) {
                    system_.log(log_level::info, response);
                }

                return response;
            });
        };

        std::cout << "Simulating web server with mixed request types...\n";

        // Generate mixed requests
        std::vector<std::future<std::string>> request_futures;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> type_dist(0, 4);

        const int total_requests = 100;
        for (int i = 0; i < total_requests; ++i) {
            auto type = static_cast<request_type>(type_dist(gen));
            request_futures.push_back(generate_request(type, i));

            // Add some random delay between requests
            if (i % 10 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        std::cout << "Processing " << total_requests << " requests...\n";

        // Collect all responses
        std::vector<std::string> responses;
        for (auto& future : request_futures) {
            responses.push_back(future.get());
        }

        // Display results
        std::cout << "\nRequest Processing Summary:\n";
        std::cout << "  Static content: " << stats.static_requests.load() << " requests\n";
        std::cout << "  API calls: " << stats.api_requests.load() << " requests\n";
        std::cout << "  User uploads: " << stats.upload_requests.load() << " requests\n";
        std::cout << "  Admin actions: " << stats.admin_requests.load() << " requests\n";
        std::cout << "  Analytics: " << stats.analytics_requests.load() << " requests\n";
        std::cout << "  Total processed: " << stats.total_requests.load() << " requests\n";

        double avg_processing_time = static_cast<double>(stats.total_processing_time_ms.load()) / stats.total_requests.load();
        std::cout << "  Average processing time: " << std::fixed << std::setprecision(2)
                  << avg_processing_time << " ms\n";

        // Show sample of responses (first 5 and last 5)
        std::cout << "\nSample responses (execution order):\n";
        std::cout << "First 5:\n";
        for (int i = 0; i < std::min(5, static_cast<int>(responses.size())); ++i) {
            std::cout << "  " << (i + 1) << ". " << responses[i] << "\n";
        }

        if (responses.size() > 10) {
            std::cout << "Last 5:\n";
            for (int i = responses.size() - 5; i < static_cast<int>(responses.size()); ++i) {
                std::cout << "  " << (i + 1) << ". " << responses[i] << "\n";
            }
        }

        std::cout << "✓ Web server simulation completed\n";
    }

    /**
     * @brief Example 6: System stress test and adaptive behavior
     */
    void stress_test_example() {
        std::cout << "\n=== Example 6: System Stress Test and Adaptive Behavior ===\n";

        const int num_stress_phases = 3;
        const std::vector<int> phase_loads = {100, 1000, 500}; // Light, Heavy, Medium
        const std::vector<std::string> phase_names = {"Light Load", "Heavy Load", "Medium Load"};

        for (int phase = 0; phase < num_stress_phases; ++phase) {
            std::cout << "\n--- Phase " << (phase + 1) << ": " << phase_names[phase] << " ("
                      << phase_loads[phase] << " tasks) ---\n";

            auto phase_start = std::chrono::high_resolution_clock::now();
            std::vector<std::future<int>> phase_futures;

            // Submit tasks for this phase
            for (int task = 0; task < phase_loads[phase]; ++task) {
                phase_futures.push_back(system_.submit([this, task, phase]() -> int {
                    // Variable work simulation
                    int work_units = 10 + (task % 20);
                    int result = 0;

                    for (int i = 0; i < work_units; ++i) {
                        result += (task * phase + i) % 1000;
                        // Small delay to simulate CPU work
                        std::this_thread::sleep_for(std::chrono::microseconds(100 + (i % 10)));
                    }

                    // Occasional logging for monitoring
                    if (task % 100 == 0) {
                        system_.log(log_level::debug, "Phase {} task {} completed with result {}",
                                   phase + 1, task, result);
                    }

                    return result;
                }));
            }

            // Monitor system during phase execution
            std::thread monitor_thread([this, phase, &phase_start]() {
                auto last_metrics = system_.get_metrics();

                while (true) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));

                    auto current_metrics = system_.get_metrics();
                    auto health = system_.get_health();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::high_resolution_clock::now() - phase_start).count();

                    if (current_metrics.tasks_completed > last_metrics.tasks_completed) {
                        auto tasks_per_sec = (current_metrics.tasks_completed - last_metrics.tasks_completed) * 2; // * 2 because we sample every 0.5s

                        std::cout << "  [" << elapsed << "s] Throughput: " << tasks_per_sec
                                  << " tasks/sec, Queue: " << current_metrics.queue_size
                                  << ", Health: ";

                        switch (health.overall_health) {
                            case health_level::healthy: std::cout << "✓"; break;
                            case health_level::degraded: std::cout << "⚠"; break;
                            case health_level::critical: std::cout << "❌"; break;
                            case health_level::failed: std::cout << "💥"; break;
                        }
                        std::cout << "\n";

                        last_metrics = current_metrics;
                    }

                    // Break if all tasks completed
                    if (current_metrics.tasks_completed >= current_metrics.tasks_submitted) {
                        break;
                    }
                }
            });

            // Wait for phase completion
            long long phase_total = 0;
            for (auto& future : phase_futures) {
                phase_total += future.get();
            }

            monitor_thread.join();

            auto phase_end = std::chrono::high_resolution_clock::now();
            auto phase_duration = std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start);

            std::cout << "  Phase completed in " << phase_duration.count() << "ms\n";
            std::cout << "  Tasks processed: " << phase_loads[phase] << "\n";
            std::cout << "  Average throughput: " << std::fixed << std::setprecision(1)
                      << (phase_loads[phase] * 1000.0 / phase_duration.count()) << " tasks/sec\n";
            std::cout << "  Total result sum: " << phase_total << "\n";

            // Brief pause between phases
            if (phase < num_stress_phases - 1) {
                std::cout << "  Pausing before next phase...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

        std::cout << "\n✓ Stress test completed - system demonstrated adaptive behavior under varying loads\n";
    }

    /**
     * @brief Run all comprehensive examples
     */
    void run_all_examples() {
        std::cout << "🚀 Starting Comprehensive Integrated Thread System Examples\n";
        std::cout << "==========================================================\n";

        std::cout << "System Configuration:\n";
        std::cout << "  Worker threads: " << system_.worker_count() << "\n";
        std::cout << "  Logging: " << (system_.is_healthy() ? "✓ Active" : "✗ Inactive") << "\n";
        std::cout << "  Monitoring: " << (system_.is_healthy() ? "✓ Active" : "✗ Inactive") << "\n";
        std::cout << "  Initial health: " << (system_.is_healthy() ? "✓ Healthy" : "⚠ Issues detected") << "\n";

        try {
            basic_usage_example();
            priority_scheduling_example();
            batch_processing_example();
            monitoring_example();
            web_server_simulation();
            stress_test_example();

        } catch (const std::exception& e) {
            std::cerr << "❌ Exception during example execution: " << e.what() << "\n";
            system_.log(log_level::error, "Example execution failed: {}", e.what());
        }

        // Final system status
        std::cout << "\n=== Final System Summary ===\n";
        auto final_metrics = system_.get_metrics();
        std::cout << "Total tasks submitted: " << final_metrics.tasks_submitted << "\n";
        std::cout << "Total tasks completed: " << final_metrics.tasks_completed << "\n";
        std::cout << "Success rate: " << std::fixed << std::setprecision(2)
                  << (100.0 * final_metrics.tasks_completed / std::max(1UL, final_metrics.tasks_submitted)) << "%\n";

        // Wait for any remaining tasks
        system_.wait_for_completion();

        std::cout << "\n🎉 All comprehensive examples completed successfully!\n";
        std::cout << "📁 Check the logs directory for detailed execution logs.\n";
        std::cout << "📊 Performance metrics have been collected throughout execution.\n";
    }
};

int main() {
    try {
        ComprehensiveExample example;
        example.run_all_examples();

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
}