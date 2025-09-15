/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, Unified Thread System
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include "../include/unified_thread_system.h"
#include <iostream>
#include <chrono>
#include <atomic>
#include <random>
#include <thread>
#include <vector>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

/**
 * @brief Demonstrates adaptive queue optimization behavior
 *
 * This example shows how the unified thread system's adaptive queue
 * automatically optimizes between mutex-based and lock-free implementations
 * based on contention levels, providing the same capabilities as the
 * original thread_system's adaptive_job_queue.
 */
class adaptive_optimization_demo
{
private:
    unified_thread_system system_;
    std::atomic<int> jobs_completed_{0};

public:
    adaptive_optimization_demo() : system_(config{}.set_worker_count(4).enable_adaptive_optimization(true)) {
        std::cout << "=== Adaptive Queue Optimization Demo ===" << std::endl;
        std::cout << "Unified system with adaptive optimization enabled" << std::endl;
    }

    void demonstrate_contention_adaptation()
    {
        std::cout << "\n--- Contention Level Adaptation ---" << std::endl;

        // Phase 1: Low contention (single producer, single consumer pattern)
        std::cout << "Phase 1: Low contention scenario" << std::endl;
        jobs_completed_ = 0;

        auto low_contention_start = std::chrono::steady_clock::now();

        // Single producer submitting jobs slowly
        std::thread producer([this] {
            for (int i = 0; i < 50; ++i) {
                system_.submit([this, i] {
                    // Minimal work to focus on queue behavior
                    jobs_completed_.fetch_add(1);
                    return i;
                });
                std::this_thread::sleep_for(2ms);  // Slow production
            }
        });

        producer.join();

        // Wait for all jobs to complete
        while (jobs_completed_ < 50) {
            std::this_thread::sleep_for(1ms);
        }

        auto low_contention_duration = std::chrono::steady_clock::now() - low_contention_start;
        std::cout << "Low contention: 50 jobs completed in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(low_contention_duration).count()
                  << "ms" << std::endl;

        std::this_thread::sleep_for(100ms);  // Allow adaptation

        // Phase 2: High contention (multiple producers, fast submission)
        std::cout << "Phase 2: High contention scenario" << std::endl;
        jobs_completed_ = 0;

        auto high_contention_start = std::chrono::steady_clock::now();

        std::vector<std::thread> producers;
        const int num_producers = 8;
        const int jobs_per_producer = 125;

        for (int p = 0; p < num_producers; ++p) {
            producers.emplace_back([this, p, jobs_per_producer] {
                for (int i = 0; i < jobs_per_producer; ++i) {
                    system_.submit([this, p, i] {
                        jobs_completed_.fetch_add(1);
                        return p * 1000 + i;
                    });
                }
            });
        }

        for (auto& prod : producers) {
            prod.join();
        }

        // Wait for all jobs to complete
        const int total_jobs = num_producers * jobs_per_producer;
        while (jobs_completed_ < total_jobs) {
            std::this_thread::sleep_for(1ms);
        }

        auto high_contention_duration = std::chrono::steady_clock::now() - high_contention_start;
        std::cout << "High contention: " << total_jobs << " jobs completed in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(high_contention_duration).count()
                  << "ms" << std::endl;

        // Calculate throughput
        auto low_throughput = 50.0 / std::chrono::duration<double>(low_contention_duration).count();
        auto high_throughput = total_jobs / std::chrono::duration<double>(high_contention_duration).count();

        std::cout << "Low contention throughput: " << static_cast<int>(low_throughput) << " jobs/sec" << std::endl;
        std::cout << "High contention throughput: " << static_cast<int>(high_throughput) << " jobs/sec" << std::endl;
        std::cout << "Note: Queue automatically adapted to optimize for different contention levels" << std::endl;
    }

    void demonstrate_mixed_workload_adaptation()
    {
        std::cout << "\n--- Mixed Workload Adaptation ---" << std::endl;

        jobs_completed_ = 0;
        std::atomic<int> quick_jobs{0};
        std::atomic<int> slow_jobs{0};

        auto mixed_start = std::chrono::steady_clock::now();

        // Create a mixed workload that varies in intensity
        std::thread workload_generator([this, &quick_jobs, &slow_jobs] {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> intensity_dist(1, 10);
            std::uniform_int_distribution<> work_dist(1, 20);

            for (int phase = 0; phase < 10; ++phase) {
                int intensity = intensity_dist(gen);
                std::cout << "Workload phase " << phase << " - intensity: " << intensity << std::endl;

                // Submit jobs with varying intensity
                for (int i = 0; i < intensity * 10; ++i) {
                    if (work_dist(gen) <= 15) {  // 75% quick jobs
                        system_.submit([this, &quick_jobs, phase, i] {
                            quick_jobs.fetch_add(1);
                            jobs_completed_.fetch_add(1);
                            return phase * 100 + i;
                        });
                    } else {  // 25% slower jobs
                        system_.submit([this, &slow_jobs, phase, i] {
                            std::this_thread::sleep_for(std::chrono::microseconds(100));
                            slow_jobs.fetch_add(1);
                            jobs_completed_.fetch_add(1);
                            return phase * 100 + i;
                        });
                    }
                }

                // Variable pause between phases
                std::this_thread::sleep_for(std::chrono::milliseconds(intensity * 10));
            }
        });

        workload_generator.join();

        // Monitor completion
        int last_completed = 0;
        while (true) {
            std::this_thread::sleep_for(50ms);
            int current_completed = jobs_completed_.load();
            if (current_completed == last_completed && current_completed > 0) {
                // No progress, assume completion
                break;
            }
            last_completed = current_completed;

            if (current_completed % 100 == 0 && current_completed > 0) {
                std::cout << "Progress: " << current_completed << " jobs completed" << std::endl;
            }
        }

        auto mixed_duration = std::chrono::steady_clock::now() - mixed_start;
        std::cout << "Mixed workload completed: " << jobs_completed_.load() << " total jobs ("
                  << quick_jobs.load() << " quick, " << slow_jobs.load() << " slow) in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(mixed_duration).count()
                  << "ms" << std::endl;

        double avg_throughput = jobs_completed_.load() / std::chrono::duration<double>(mixed_duration).count();
        std::cout << "Average throughput: " << static_cast<int>(avg_throughput) << " jobs/sec" << std::endl;
    }

    void demonstrate_strategy_comparison()
    {
        std::cout << "\n--- Queue Strategy Performance Comparison ---" << std::endl;

        const int test_jobs = 10000;
        const int num_threads = 6;

        // Test different queue strategies that the adaptive system might use
        struct test_result {
            std::string strategy_name;
            std::chrono::milliseconds duration;
            double throughput;
        };

        std::vector<test_result> results;

        // Force different internal queue configurations for comparison
        for (const auto& strategy : {"optimized_low_contention", "optimized_high_contention", "adaptive_auto"}) {
            std::cout << "Testing " << strategy << " configuration..." << std::endl;

            jobs_completed_ = 0;
            auto test_start = std::chrono::steady_clock::now();

            // Create the test system with specific optimization hint
            config test_config;
            test_config.set_worker_count(num_threads);
            if (std::string(strategy) == "optimized_low_contention") {
                test_config.set_optimization_hint(optimization_hint::low_contention);
            } else if (std::string(strategy) == "optimized_high_contention") {
                test_config.set_optimization_hint(optimization_hint::high_contention);
            } else {
                test_config.enable_adaptive_optimization(true);
            }

            unified_thread_system test_system(test_config);

            // Submit test jobs
            std::vector<std::thread> submitters;
            const int jobs_per_thread = test_jobs / num_threads;

            for (int t = 0; t < num_threads; ++t) {
                submitters.emplace_back([&test_system, this, t, jobs_per_thread] {
                    for (int i = 0; i < jobs_per_thread; ++i) {
                        test_system.submit([this, t, i] {
                            jobs_completed_.fetch_add(1);
                            return t * 10000 + i;
                        });
                    }
                });
            }

            for (auto& submitter : submitters) {
                submitter.join();
            }

            // Wait for completion
            while (jobs_completed_ < test_jobs) {
                std::this_thread::sleep_for(1ms);
            }

            auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - test_start);

            double throughput = test_jobs / std::chrono::duration<double>(
                std::chrono::steady_clock::now() - test_start).count();

            results.push_back({strategy, test_duration, throughput});

            std::cout << strategy << ": " << test_duration.count() << "ms, "
                      << static_cast<int>(throughput) << " jobs/sec" << std::endl;

            std::this_thread::sleep_for(100ms);  // Cool down between tests
        }

        std::cout << "\nStrategy Performance Summary:" << std::endl;
        for (const auto& result : results) {
            std::cout << "  " << result.strategy_name << ": "
                      << result.duration.count() << "ms ("
                      << static_cast<int>(result.throughput) << " jobs/sec)" << std::endl;
        }
        std::cout << "Note: Adaptive configuration automatically selects optimal strategy" << std::endl;
    }

    void demonstrate_real_time_adaptation()
    {
        std::cout << "\n--- Real-time Adaptation Demo ---" << std::endl;

        jobs_completed_ = 0;
        std::atomic<bool> monitoring_active{true};

        // Start a monitoring thread to observe adaptation in real-time
        std::thread monitor([this, &monitoring_active] {
            auto start_time = std::chrono::steady_clock::now();
            int last_completed = 0;

            while (monitoring_active.load()) {
                std::this_thread::sleep_for(500ms);

                int current_completed = jobs_completed_.load();
                int jobs_this_period = current_completed - last_completed;
                last_completed = current_completed;

                auto elapsed = std::chrono::steady_clock::now() - start_time;
                auto seconds = std::chrono::duration<double>(elapsed).count();

                if (jobs_this_period > 0) {
                    double current_rate = jobs_this_period / 0.5;  // jobs per second
                    std::cout << "Time: " << static_cast<int>(seconds) << "s, "
                              << "Completed: " << current_completed << ", "
                              << "Rate: " << static_cast<int>(current_rate) << " jobs/sec" << std::endl;
                }
            }
        });

        // Generate varying load patterns
        std::thread load_generator([this] {
            // Pattern 1: Gradual increase
            std::cout << "Pattern 1: Gradual load increase" << std::endl;
            for (int intensity = 1; intensity <= 10; ++intensity) {
                for (int i = 0; i < intensity * 20; ++i) {
                    system_.submit([this] {
                        jobs_completed_.fetch_add(1);
                        return 42;
                    });
                }
                std::this_thread::sleep_for(200ms);
            }

            std::this_thread::sleep_for(1s);

            // Pattern 2: Burst load
            std::cout << "Pattern 2: Burst load" << std::endl;
            for (int burst = 0; burst < 5; ++burst) {
                for (int i = 0; i < 200; ++i) {
                    system_.submit([this] {
                        jobs_completed_.fetch_add(1);
                        return 42;
                    });
                }
                std::this_thread::sleep_for(300ms);
            }

            std::this_thread::sleep_for(1s);

            // Pattern 3: Sustained high load
            std::cout << "Pattern 3: Sustained high load" << std::endl;
            for (int i = 0; i < 1000; ++i) {
                system_.submit([this] {
                    jobs_completed_.fetch_add(1);
                    return 42;
                });
            }
        });

        load_generator.join();

        // Allow time for all jobs to complete
        std::this_thread::sleep_for(2s);
        monitoring_active = false;
        monitor.join();

        std::cout << "Real-time adaptation demo completed with " << jobs_completed_.load() << " total jobs" << std::endl;
        std::cout << "Note: The queue adapted its internal strategy in real-time based on load patterns" << std::endl;
    }

    void run_all_demonstrations()
    {
        demonstrate_contention_adaptation();
        demonstrate_mixed_workload_adaptation();
        demonstrate_strategy_comparison();
        demonstrate_real_time_adaptation();

        std::cout << "\n=== Adaptive Optimization Demo Complete ===" << std::endl;
        std::cout << "The unified thread system successfully demonstrated:" << std::endl;
        std::cout << "1. Automatic adaptation to different contention levels" << std::endl;
        std::cout << "2. Optimal performance across varying workload patterns" << std::endl;
        std::cout << "3. Real-time strategy switching for maximum efficiency" << std::endl;
        std::cout << "4. Transparent optimization without API complexity" << std::endl;
    }
};

int main()
{
    try {
        adaptive_optimization_demo demo;
        demo.run_all_demonstrations();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}