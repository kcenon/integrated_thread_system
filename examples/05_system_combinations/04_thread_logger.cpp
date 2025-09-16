/**
 * @file 04_thread_logger.cpp
 * @brief Using thread_system with logger_system (no monitoring)
 * @description Parallel processing with comprehensive logging
 */

#include "unified_thread_system.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

/**
 * Thread+Logger configuration is ideal for:
 * - Application servers with logging
 * - Background job processors
 * - Data pipeline systems
 * - Microservices with audit trails
 * - Distributed computing nodes
 */
class thread_logger_demo {
private:
    unified_thread_system system_;
    std::atomic<int> tasks_processed_{0};

public:
    thread_logger_demo() {
        // Configure for thread + logger operation
        config cfg;
        cfg.enable_thread_system(true)       // Enable threading
           .enable_logger_system(true)        // Enable logging
           .enable_monitoring_system(false)   // Disable monitoring
           .set_worker_count(4)
           .set_queue_capacity(1000)
           .set_log_level(log_level::info)
           .set_log_file("thread_logger.log")
           .enable_async_logging(true);       // Async logging with threads

        system_ = unified_thread_system(cfg);

        std::cout << "=== Thread + Logger Configuration ===" << std::endl;
        std::cout << "✓ Thread System: ENABLED (4 workers)" << std::endl;
        std::cout << "✓ Logger System: ENABLED (async)" << std::endl;
        std::cout << "✗ Monitoring System: DISABLED" << std::endl;
        std::cout << std::endl;
    }

    void demonstrate_parallel_processing_with_logging() {
        std::cout << "1. Parallel Processing with Logging:" << std::endl;

        system_.log_info("Starting parallel batch processing");

        const int num_batches = 10;
        std::vector<std::future<int>> futures;

        for (int batch = 0; batch < num_batches; ++batch) {
            futures.push_back(system_.submit([this, batch]() {
                system_.log_debug("Processing batch {}", batch);

                // Simulate batch processing
                int items_processed = 0;
                for (int i = 0; i < 100; ++i) {
                    // Process item
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    items_processed++;

                    if (i % 25 == 0) {
                        system_.log_debug("Batch {}: {}% complete", batch, i);
                    }
                }

                system_.log_info("Batch {} completed: {} items processed",
                    batch, items_processed);

                tasks_processed_.fetch_add(items_processed);
                return items_processed;
            }));
        }

        // Wait and collect results
        int total = 0;
        for (auto& future : futures) {
            total += future.get();
        }

        system_.log_info("Batch processing complete: {} total items", total);
        std::cout << "   Processed " << total << " items across " << num_batches
                  << " batches (check logs for details)" << std::endl;
    }

    void demonstrate_error_handling_with_logs() {
        std::cout << "\n2. Error Handling with Logging:" << std::endl;

        system_.log_info("Starting error handling demonstration");

        auto safe_divide = [this](double a, double b) -> std::future<double> {
            return system_.submit([this, a, b]() -> double {
                system_.log_debug("Division operation: {} / {}", a, b);

                if (b == 0) {
                    system_.log_error("Division by zero attempted: {} / {}",
                        a, b,
                        {{"error_type", "arithmetic_error"},
                         {"severity", "high"}});
                    throw std::invalid_argument("Division by zero");
                }

                double result = a / b;
                system_.log_info("Division successful: {} / {} = {}", a, b, result);
                return result;
            });
        };

        // Test various operations
        std::vector<std::pair<double, double>> operations = {
            {10.0, 2.0},
            {15.0, 0.0},  // Will cause error
            {20.0, 4.0},
            {8.0, 0.0},   // Will cause error
            {100.0, 25.0}
        };

        for (const auto& [a, b] : operations) {
            try {
                auto future = safe_divide(a, b);
                double result = future.get();
                std::cout << "   " << a << " / " << b << " = " << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "   " << a << " / " << b << " = ERROR: " << e.what() << std::endl;
            }
        }
    }

    void demonstrate_pipeline_with_logging() {
        std::cout << "\n3. Data Pipeline with Stage Logging:" << std::endl;

        system_.log_info("Initializing data pipeline");

        struct pipeline_data {
            int id;
            std::vector<int> values;
            std::string status;
        };

        // Stage 1: Data generation
        auto stage1 = system_.submit([this]() {
            system_.log_info("Pipeline Stage 1: Data Generation");

            std::vector<pipeline_data> data;
            for (int i = 0; i < 5; ++i) {
                pipeline_data item;
                item.id = i;
                item.values.resize(10);
                std::iota(item.values.begin(), item.values.end(), i * 10);
                item.status = "generated";
                data.push_back(item);

                system_.log_debug("Generated data item {}", i);
            }

            system_.log_info("Stage 1 complete: {} items generated", data.size());
            return data;
        });

        // Stage 2: Data transformation
        auto data = stage1.get();
        std::vector<std::future<pipeline_data>> stage2_futures;

        system_.log_info("Pipeline Stage 2: Parallel Transformation");

        for (auto& item : data) {
            stage2_futures.push_back(system_.submit([this, item]() mutable {
                system_.log_debug("Transforming item {}", item.id);

                // Transform data
                for (auto& val : item.values) {
                    val = val * 2 + 1;
                }
                item.status = "transformed";

                system_.log_debug("Item {} transformed", item.id);
                return item;
            }));
        }

        // Stage 3: Data aggregation
        system_.log_info("Pipeline Stage 3: Aggregation");

        auto stage3 = system_.submit([this, &stage2_futures]() {
            int total_sum = 0;
            int items_processed = 0;

            for (auto& future : stage2_futures) {
                auto item = future.get();
                int item_sum = std::accumulate(item.values.begin(), item.values.end(), 0);
                total_sum += item_sum;
                items_processed++;

                system_.log_debug("Aggregated item {}: sum = {}", item.id, item_sum);
            }

            system_.log_info("Stage 3 complete: {} items aggregated, total = {}",
                items_processed, total_sum);

            return total_sum;
        });

        int final_result = stage3.get();
        system_.log_info("Pipeline complete: final result = {}", final_result);

        std::cout << "   Pipeline processed " << data.size() << " items" << std::endl;
        std::cout << "   Final aggregated result: " << final_result << std::endl;
        std::cout << "   (Check logs for detailed pipeline execution)" << std::endl;
    }

    void demonstrate_concurrent_logging() {
        std::cout << "\n4. Concurrent Logging Performance:" << std::endl;

        system_.log_info("Testing concurrent logging performance");

        const int num_threads = 10;
        const int logs_per_thread = 100;

        auto start = std::chrono::steady_clock::now();

        std::vector<std::future<void>> futures;
        for (int t = 0; t < num_threads; ++t) {
            futures.push_back(system_.submit([this, t, logs_per_thread]() {
                for (int i = 0; i < logs_per_thread; ++i) {
                    system_.log_info("Thread {}: Log entry {}",
                        t, i,
                        {{"thread_id", std::this_thread::get_id()},
                         {"sequence", i},
                         {"timestamp", std::chrono::system_clock::now()}});
                }
            }));
        }

        // Wait for all logging to complete
        for (auto& future : futures) {
            future.wait();
        }

        auto duration = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        system_.log_info("Concurrent logging test complete: {} logs in {}ms",
            num_threads * logs_per_thread, ms);

        std::cout << "   Generated " << num_threads * logs_per_thread << " log entries" << std::endl;
        std::cout << "   From " << num_threads << " concurrent threads" << std::endl;
        std::cout << "   Completed in " << ms << "ms" << std::endl;
        std::cout << "   Throughput: " << (num_threads * logs_per_thread * 1000 / ms)
                  << " logs/sec" << std::endl;
    }

    void demonstrate_task_tracking() {
        std::cout << "\n5. Task Execution Tracking:" << std::endl;

        system_.log_info("Starting task execution tracking");

        // Task wrapper that logs execution
        auto tracked_task = [this](const std::string& task_name, auto func) {
            return system_.submit([this, task_name, func]() {
                auto task_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

                system_.log_info("Task started",
                    {{"task_name", task_name},
                     {"task_id", task_id}});

                auto start = std::chrono::steady_clock::now();

                try {
                    auto result = func();

                    auto duration = std::chrono::steady_clock::now() - start;
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

                    system_.log_info("Task completed",
                        {{"task_name", task_name},
                         {"task_id", task_id},
                         {"duration_ms", ms},
                         {"status", "success"}});

                    return result;
                } catch (const std::exception& e) {
                    auto duration = std::chrono::steady_clock::now() - start;
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

                    system_.log_error("Task failed",
                        {{"task_name", task_name},
                         {"task_id", task_id},
                         {"duration_ms", ms},
                         {"error", e.what()}});

                    throw;
                }
            });
        };

        // Execute tracked tasks
        auto task1 = tracked_task("calculate_sum", []() {
            std::this_thread::sleep_for(50ms);
            return 42;
        });

        auto task2 = tracked_task("process_data", []() {
            std::this_thread::sleep_for(75ms);
            return std::string("processed");
        });

        auto task3 = tracked_task("validate_input", []() {
            std::this_thread::sleep_for(25ms);
            return true;
        });

        // Get results
        std::cout << "   Task 'calculate_sum' result: " << task1.get() << std::endl;
        std::cout << "   Task 'process_data' result: " << task2.get() << std::endl;
        std::cout << "   Task 'validate_input' result: " << task3.get() << std::endl;
        std::cout << "   (Task execution details in logs)" << std::endl;
    }

    void demonstrate_audit_trail() {
        std::cout << "\n6. Audit Trail with Parallel Operations:" << std::endl;

        system_.log_info("AUDIT: Session started",
            {{"session_id", "sess_12345"},
             {"user", "admin"}});

        // Simulate user operations in parallel
        struct user_operation {
            std::string operation;
            std::string resource;
            bool allowed;
        };

        std::vector<user_operation> operations = {
            {"READ", "user_profiles", true},
            {"WRITE", "system_config", false},
            {"DELETE", "temp_files", true},
            {"CREATE", "report", true},
            {"MODIFY", "permissions", false}
        };

        std::vector<std::future<bool>> operation_futures;

        for (const auto& op : operations) {
            operation_futures.push_back(system_.submit([this, op]() {
                system_.log_info("AUDIT: Operation attempted",
                    {{"operation", op.operation},
                     {"resource", op.resource},
                     {"allowed", op.allowed}});

                if (!op.allowed) {
                    system_.log_warning("AUDIT: Operation denied",
                        {{"operation", op.operation},
                         {"resource", op.resource},
                         {"reason", "insufficient_permissions"}});
                    return false;
                }

                // Simulate operation execution
                std::this_thread::sleep_for(20ms);

                system_.log_info("AUDIT: Operation completed",
                    {{"operation", op.operation},
                     {"resource", op.resource}});

                return true;
            }));
        }

        // Collect results
        int successful = 0;
        int denied = 0;

        for (auto& future : operation_futures) {
            if (future.get()) {
                successful++;
            } else {
                denied++;
            }
        }

        system_.log_info("AUDIT: Session summary",
            {{"successful_operations", successful},
             {"denied_operations", denied},
             {"total_operations", operations.size()}});

        std::cout << "   Audit trail created:" << std::endl;
        std::cout << "   - Successful operations: " << successful << std::endl;
        std::cout << "   - Denied operations: " << denied << std::endl;
        std::cout << "   (Full audit trail in logs)" << std::endl;
    }

    void run_all_demonstrations() {
        demonstrate_parallel_processing_with_logging();
        demonstrate_error_handling_with_logs();
        demonstrate_pipeline_with_logging();
        demonstrate_concurrent_logging();
        demonstrate_task_tracking();
        demonstrate_audit_trail();

        std::cout << "\n=== Thread + Logger Benefits ===" << std::endl;
        std::cout << "✓ Parallel execution with detailed logging" << std::endl;
        std::cout << "✓ Async logging doesn't block workers" << std::endl;
        std::cout << "✓ Complete execution audit trail" << std::endl;
        std::cout << "✓ Error tracking across threads" << std::endl;
        std::cout << "✓ Pipeline stage visibility" << std::endl;

        std::cout << "\nTotal tasks processed: " << tasks_processed_.load() << std::endl;
    }
};

int main() {
    try {
        thread_logger_demo demo;
        demo.run_all_demonstrations();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * When to use Thread+Logger configuration:
 *
 * 1. Application Servers
 *    - Web services
 *    - API servers
 *    - Background job processors
 *
 * 2. Data Processing Systems
 *    - ETL pipelines
 *    - Batch processors
 *    - Stream processors
 *
 * 3. Distributed Systems
 *    - Worker nodes
 *    - Task executors
 *    - Message processors
 *
 * 4. Development & Debugging
 *    - Performance analysis
 *    - Error investigation
 *    - Execution tracing
 */