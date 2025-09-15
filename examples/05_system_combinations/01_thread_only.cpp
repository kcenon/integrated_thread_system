/**
 * @file 01_thread_only.cpp
 * @brief Using only the thread_system without logger or monitoring
 * @description Minimal configuration for pure threading functionality
 */

#include "unified_thread_system.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>

using namespace integrated_thread_system;
using namespace std::chrono_literals;

/**
 * Thread-only configuration is ideal for:
 * - Lightweight applications
 * - Embedded systems with limited resources
 * - Pure computational tasks without logging needs
 * - Testing and benchmarking scenarios
 */
class thread_only_demo {
private:
    unified_thread_system system_;

public:
    thread_only_demo() {
        // Configure for thread-only operation
        config cfg;
        cfg.enable_thread_system(true)
           .enable_logger_system(false)    // Explicitly disable logger
           .enable_monitoring_system(false) // Explicitly disable monitoring
           .set_worker_count(std::thread::hardware_concurrency())
           .set_queue_capacity(1000);

        system_ = unified_thread_system(cfg);

        std::cout << "=== Thread-Only Configuration ===" << std::endl;
        std::cout << "✓ Thread System: ENABLED" << std::endl;
        std::cout << "✗ Logger System: DISABLED" << std::endl;
        std::cout << "✗ Monitoring System: DISABLED" << std::endl;
        std::cout << "Workers: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << std::endl;
    }

    void demonstrate_pure_computation() {
        std::cout << "1. Pure Computational Tasks:" << std::endl;

        // Matrix multiplication example
        const int size = 100;
        std::vector<std::vector<double>> matrix_a(size, std::vector<double>(size, 1.5));
        std::vector<std::vector<double>> matrix_b(size, std::vector<double>(size, 2.0));

        auto start = std::chrono::steady_clock::now();

        // Parallel matrix multiplication using thread blocks
        std::vector<std::future<std::vector<double>>> futures;

        for (int i = 0; i < size; ++i) {
            futures.push_back(system_.submit([&matrix_a, &matrix_b, i, size]() {
                std::vector<double> row(size, 0.0);
                for (int j = 0; j < size; ++j) {
                    for (int k = 0; k < size; ++k) {
                        row[j] += matrix_a[i][k] * matrix_b[k][j];
                    }
                }
                return row;
            }));
        }

        // Collect results
        std::vector<std::vector<double>> result;
        for (auto& future : futures) {
            result.push_back(future.get());
        }

        auto duration = std::chrono::steady_clock::now() - start;
        std::cout << "   Matrix multiplication (" << size << "x" << size << ") completed in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << "ms" << std::endl;
    }

    void demonstrate_parallel_algorithms() {
        std::cout << "\n2. Parallel Algorithms:" << std::endl;

        // Parallel sorting example
        std::vector<int> data(1000000);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000000);

        for (auto& val : data) {
            val = dis(gen);
        }

        auto start = std::chrono::steady_clock::now();

        // Split data for parallel sorting
        const int num_chunks = 4;
        const int chunk_size = data.size() / num_chunks;
        std::vector<std::future<std::vector<int>>> sort_futures;

        for (int i = 0; i < num_chunks; ++i) {
            auto begin_it = data.begin() + i * chunk_size;
            auto end_it = (i == num_chunks - 1) ? data.end() : begin_it + chunk_size;

            sort_futures.push_back(system_.submit([begin_it, end_it]() {
                std::vector<int> chunk(begin_it, end_it);
                std::sort(chunk.begin(), chunk.end());
                return chunk;
            }));
        }

        // Merge sorted chunks
        std::vector<std::vector<int>> sorted_chunks;
        for (auto& future : sort_futures) {
            sorted_chunks.push_back(future.get());
        }

        // Final merge (simplified - in production use proper merge)
        std::vector<int> final_result;
        for (const auto& chunk : sorted_chunks) {
            final_result.insert(final_result.end(), chunk.begin(), chunk.end());
        }
        std::sort(final_result.begin(), final_result.end());

        auto duration = std::chrono::steady_clock::now() - start;
        std::cout << "   Parallel sort of " << data.size() << " elements completed in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << "ms" << std::endl;
    }

    void demonstrate_map_reduce() {
        std::cout << "\n3. Map-Reduce Pattern:" << std::endl;

        // Calculate sum of squares using map-reduce
        std::vector<int> numbers(100000);
        std::iota(numbers.begin(), numbers.end(), 1);

        auto start = std::chrono::steady_clock::now();

        // Map phase: square each number
        const int chunk_size = 10000;
        std::vector<std::future<long long>> map_futures;

        for (size_t i = 0; i < numbers.size(); i += chunk_size) {
            size_t end = std::min(i + chunk_size, numbers.size());

            map_futures.push_back(system_.submit([&numbers, i, end]() {
                long long sum = 0;
                for (size_t j = i; j < end; ++j) {
                    sum += static_cast<long long>(numbers[j]) * numbers[j];
                }
                return sum;
            }));
        }

        // Reduce phase: sum all partial results
        long long total = 0;
        for (auto& future : map_futures) {
            total += future.get();
        }

        auto duration = std::chrono::steady_clock::now() - start;
        std::cout << "   Sum of squares (1 to " << numbers.size() << "): " << total << std::endl;
        std::cout << "   Completed in "
                  << std::chrono::duration_cast<std::chrono::microseconds>(duration).count()
                  << "μs" << std::endl;
    }

    void demonstrate_pipeline_processing() {
        std::cout << "\n4. Pipeline Processing:" << std::endl;

        // Three-stage pipeline: generate -> transform -> aggregate
        const int num_items = 1000;

        auto start = std::chrono::steady_clock::now();

        // Stage 1: Generate data
        auto generation_future = system_.submit([num_items]() {
            std::vector<int> data;
            data.reserve(num_items);
            for (int i = 0; i < num_items; ++i) {
                data.push_back(i + 1);
            }
            return data;
        });

        // Stage 2: Transform data (wait for generation)
        auto data = generation_future.get();

        std::vector<std::future<int>> transform_futures;
        for (int val : data) {
            transform_futures.push_back(system_.submit([val]() {
                // Simulate complex transformation
                return val * val + val / 2;
            }));
        }

        // Stage 3: Aggregate results
        auto aggregate_future = system_.submit([&transform_futures]() {
            int sum = 0;
            for (auto& future : transform_futures) {
                sum += future.get();
            }
            return sum;
        });

        int result = aggregate_future.get();
        auto duration = std::chrono::steady_clock::now() - start;

        std::cout << "   Pipeline result: " << result << std::endl;
        std::cout << "   Pipeline completed in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << "ms" << std::endl;
    }

    void demonstrate_resource_efficiency() {
        std::cout << "\n5. Resource Efficiency Metrics:" << std::endl;

        // Measure memory footprint
        std::cout << "   Memory footprint: MINIMAL (no logger/monitor overhead)" << std::endl;

        // Measure throughput
        const int num_tasks = 10000;
        auto start = std::chrono::steady_clock::now();

        std::vector<std::future<int>> futures;
        for (int i = 0; i < num_tasks; ++i) {
            futures.push_back(system_.submit([i]() { return i * 2; }));
        }

        for (auto& future : futures) {
            future.get();
        }

        auto duration = std::chrono::steady_clock::now() - start;
        double throughput = num_tasks * 1000.0 /
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        std::cout << "   Throughput: " << static_cast<int>(throughput) << " tasks/sec" << std::endl;
        std::cout << "   Latency: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / num_tasks
                  << "μs per task" << std::endl;
    }

    void run_all_demonstrations() {
        demonstrate_pure_computation();
        demonstrate_parallel_algorithms();
        demonstrate_map_reduce();
        demonstrate_pipeline_processing();
        demonstrate_resource_efficiency();

        std::cout << "\n=== Thread-Only Benefits ===" << std::endl;
        std::cout << "✓ Minimal memory overhead" << std::endl;
        std::cout << "✓ Maximum computational throughput" << std::endl;
        std::cout << "✓ No I/O overhead from logging" << std::endl;
        std::cout << "✓ No monitoring overhead" << std::endl;
        std::cout << "✓ Ideal for embedded systems" << std::endl;
    }
};

int main() {
    try {
        thread_only_demo demo;
        demo.run_all_demonstrations();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * When to use Thread-Only configuration:
 *
 * 1. High-Performance Computing (HPC)
 *    - Scientific simulations
 *    - Numerical computations
 *    - Machine learning inference
 *
 * 2. Embedded Systems
 *    - Limited memory environments
 *    - Real-time systems
 *    - IoT devices
 *
 * 3. Microservices
 *    - Simple computational services
 *    - Stateless processors
 *    - Lambda functions
 *
 * 4. Testing & Benchmarking
 *    - Performance testing
 *    - Algorithm validation
 *    - Stress testing
 */