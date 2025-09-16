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

#include <unified_thread_system.h>
#include <iostream>
#include <chrono>
#include <atomic>
#include <random>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

/**
 * @brief Demonstrates typed thread pool functionality with priority-based scheduling
 *
 * This example shows how the unified thread system provides the same
 * typed_thread_pool capabilities as the original thread_system, but with
 * simplified initialization and usage.
 */
class typed_thread_pool_demo
{
private:
    unified_thread_system system_;
    std::atomic<int> critical_completed_{0};
    std::atomic<int> normal_completed_{0};
    std::atomic<int> background_completed_{0};

public:
    typed_thread_pool_demo() : system_(config{}.set_worker_count(8)) {
        std::cout << "=== Typed Thread Pool Demo ===" << std::endl;
        std::cout << "Unified system initialized with priority-based scheduling" << std::endl;
    }

    void demonstrate_priority_scheduling()
    {
        std::cout << "\n--- Priority Scheduling Demo ---" << std::endl;

        // Reset counters
        critical_completed_ = 0;
        normal_completed_ = 0;
        background_completed_ = 0;

        auto start_time = std::chrono::steady_clock::now();

        // Submit jobs with different priorities in mixed order
        // Background jobs (should be processed last)
        for (int i = 0; i < 5; ++i) {
            system_.submit_background([this, i] {
                std::this_thread::sleep_for(10ms);  // Simulate work
                background_completed_.fetch_add(1);
                std::cout << "BACKGROUND job " << i << " completed" << std::endl;
                return i;
            });
        }

        // Normal priority jobs (processed in middle)
        for (int i = 0; i < 5; ++i) {
            system_.submit([this, i] {
                std::this_thread::sleep_for(5ms);  // Less work than background
                normal_completed_.fetch_add(1);
                std::cout << "NORMAL job " << i << " completed" << std::endl;
                return i * 2;
            });
        }

        // Critical jobs (should be processed first)
        for (int i = 0; i < 3; ++i) {
            system_.submit_critical([this, i] {
                std::this_thread::sleep_for(1ms);  // Quick critical work
                critical_completed_.fetch_add(1);
                std::cout << "CRITICAL job " << i << " completed" << std::endl;
                return i * 10;
            });
        }

        // Wait for all jobs to complete
        while (critical_completed_ < 3 || normal_completed_ < 5 || background_completed_ < 5) {
            std::this_thread::sleep_for(10ms);
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << "All jobs completed in " << duration.count() << "ms" << std::endl;
        std::cout << "Note: Critical jobs should appear first, then normal, then background" << std::endl;
    }

    void demonstrate_batch_processing()
    {
        std::cout << "\n--- Batch Processing Demo ---" << std::endl;

        // Simulate processing a batch of data items with different priorities
        struct data_item {
            int id;
            std::string priority_level;
            int processing_time_ms;
        };

        std::vector<data_item> items = {
            {1, "critical", 5},
            {2, "normal", 20},
            {3, "background", 50},
            {4, "critical", 3},
            {5, "normal", 25},
            {6, "background", 45},
            {7, "critical", 2},
            {8, "normal", 30},
        };

        std::cout << "Processing " << items.size() << " data items with mixed priorities..." << std::endl;

        std::vector<std::future<int>> futures;
        auto batch_start = std::chrono::steady_clock::now();

        for (const auto& item : items) {
            if (item.priority_level == "critical") {
                futures.push_back(system_.submit_critical([item] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(item.processing_time_ms));
                    std::cout << "Processed CRITICAL item " << item.id << " (" << item.processing_time_ms << "ms)" << std::endl;
                    return item.id;
                }));
            } else if (item.priority_level == "normal") {
                futures.push_back(system_.submit([item] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(item.processing_time_ms));
                    std::cout << "Processed NORMAL item " << item.id << " (" << item.processing_time_ms << "ms)" << std::endl;
                    return item.id;
                }));
            } else {
                futures.push_back(system_.submit_background([item] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(item.processing_time_ms));
                    std::cout << "Processed BACKGROUND item " << item.id << " (" << item.processing_time_ms << "ms)" << std::endl;
                    return item.id;
                }));
            }
        }

        // Wait for all items to be processed
        for (auto& future : futures) {
            future.wait();
        }

        auto batch_end = std::chrono::steady_clock::now();
        auto batch_duration = std::chrono::duration_cast<std::chrono::milliseconds>(batch_end - batch_start);
        std::cout << "Batch processing completed in " << batch_duration.count() << "ms" << std::endl;
    }

    void demonstrate_adaptive_behavior()
    {
        std::cout << "\n--- Adaptive Queue Behavior Demo ---" << std::endl;

        // First phase: Low contention
        std::cout << "Phase 1: Low contention scenario..." << std::endl;
        auto phase1_start = std::chrono::steady_clock::now();

        std::vector<std::future<void>> phase1_futures;
        for (int i = 0; i < 10; ++i) {
            phase1_futures.push_back(system_.submit([i] {
                std::this_thread::sleep_for(5ms);
                if (i % 5 == 0) {
                    std::cout << "Low contention job " << i << " completed" << std::endl;
                }
            }));
        }

        for (auto& f : phase1_futures) f.wait();
        auto phase1_duration = std::chrono::steady_clock::now() - phase1_start;
        std::cout << "Phase 1 completed in " <<
            std::chrono::duration_cast<std::chrono::milliseconds>(phase1_duration).count() << "ms" << std::endl;

        // Second phase: High contention
        std::cout << "Phase 2: High contention scenario..." << std::endl;
        auto phase2_start = std::chrono::steady_clock::now();

        std::vector<std::future<void>> phase2_futures;
        std::atomic<int> high_contention_counter{0};

        for (int i = 0; i < 100; ++i) {
            phase2_futures.push_back(system_.submit([&high_contention_counter, i] {
                // Simulate high contention work
                int count = high_contention_counter.fetch_add(1);
                if (count % 20 == 0) {
                    std::cout << "High contention: processed " << count << " jobs" << std::endl;
                }
            }));
        }

        for (auto& f : phase2_futures) f.wait();
        auto phase2_duration = std::chrono::steady_clock::now() - phase2_start;
        std::cout << "Phase 2 completed in " <<
            std::chrono::duration_cast<std::chrono::milliseconds>(phase2_duration).count() << "ms" << std::endl;

        std::cout << "Note: The adaptive queue automatically optimizes for different contention levels" << std::endl;
    }

    void demonstrate_real_world_scenario()
    {
        std::cout << "\n--- Real World Scenario: Image Processing Service ---" << std::endl;

        enum class image_priority { thumbnail, standard, high_quality };

        struct image_job {
            int id;
            image_priority priority;
            std::string filename;
            int processing_time_ms;
        };

        std::vector<image_job> image_queue = {
            {1, image_priority::high_quality, "portrait.raw", 200},
            {2, image_priority::thumbnail, "thumb1.jpg", 10},
            {3, image_priority::standard, "photo1.jpg", 50},
            {4, image_priority::thumbnail, "thumb2.jpg", 8},
            {5, image_priority::high_quality, "landscape.raw", 180},
            {6, image_priority::standard, "photo2.jpg", 45},
            {7, image_priority::thumbnail, "thumb3.jpg", 12},
            {8, image_priority::high_quality, "wedding.raw", 220},
        };

        std::cout << "Processing " << image_queue.size() << " images with different priorities..." << std::endl;

        auto processing_start = std::chrono::steady_clock::now();
        std::vector<std::future<std::string>> image_futures;

        for (const auto& job : image_queue) {
            auto process_image = [job] {
                std::this_thread::sleep_for(std::chrono::milliseconds(job.processing_time_ms));

                std::string priority_str;
                switch (job.priority) {
                    case image_priority::thumbnail: priority_str = "THUMBNAIL"; break;
                    case image_priority::standard: priority_str = "STANDARD"; break;
                    case image_priority::high_quality: priority_str = "HIGH_QUALITY"; break;
                }

                std::string result = "Processed " + priority_str + " image: " + job.filename;
                std::cout << result << std::endl;
                return result;
            };

            switch (job.priority) {
                case image_priority::thumbnail:
                    // Thumbnails need quick response - use critical priority
                    image_futures.push_back(system_.submit_critical(process_image));
                    break;
                case image_priority::standard:
                    // Standard processing - normal priority
                    image_futures.push_back(system_.submit(process_image));
                    break;
                case image_priority::high_quality:
                    // High quality can wait - background priority
                    image_futures.push_back(system_.submit_background(process_image));
                    break;
            }
        }

        // Wait for all images to be processed
        for (auto& future : image_futures) {
            future.wait();
        }

        auto processing_end = std::chrono::steady_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(processing_end - processing_start);

        std::cout << "Image processing service completed in " << total_time.count() << "ms" << std::endl;
        std::cout << "Note: Thumbnails (critical) processed first for quick user feedback" << std::endl;
    }

    void run_all_demonstrations()
    {
        demonstrate_priority_scheduling();
        demonstrate_batch_processing();
        demonstrate_adaptive_behavior();
        demonstrate_real_world_scenario();

        std::cout << "\n=== Demo Complete ===" << std::endl;
        std::cout << "The unified thread system successfully demonstrates:" << std::endl;
        std::cout << "1. Priority-based job scheduling (typed_thread_pool functionality)" << std::endl;
        std::cout << "2. Adaptive queue optimization for different contention levels" << std::endl;
        std::cout << "3. Real-world scenario handling with mixed priorities" << std::endl;
        std::cout << "4. Simple API that hides complex implementation details" << std::endl;
    }
};

int main()
{
    try {
        typed_thread_pool_demo demo;
        demo.run_all_demonstrations();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}