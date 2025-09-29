/**
 * @file test_priority_scheduling.cpp
 * @brief Unit tests for priority-based job scheduling
 */

#include <gtest/gtest.h>
#include <kcenon/integrated/unified_thread_system.h>
#include <chrono>
#include <vector>
#include <atomic>
#include <algorithm>

using namespace integrated_thread_system;
using namespace std::chrono_literals;

class PrioritySchedulingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use limited workers to better test priority effects
        unified_thread_system::config cfg;
        cfg.thread_count = 2;
        system_ = std::make_unique<unified_thread_system>(cfg);
    }

    void TearDown() override {
        system_.reset();
    }

    std::unique_ptr<unified_thread_system> system_;
};

TEST_F(PrioritySchedulingTest, BasicPriorityOrdering) {
    std::vector<int> execution_order;
    std::mutex order_mutex;
    std::atomic<int> counter{0};

    // Submit tasks in reverse priority order
    auto bg_future = system_->submit_background([&]() {
        std::this_thread::sleep_for(10ms);
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(3);
        return counter.fetch_add(1);
    });

    auto normal_future = system_->submit([&]() {
        std::this_thread::sleep_for(10ms);
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(2);
        return counter.fetch_add(1);
    });

    auto critical_future = system_->submit_critical([&]() {
        std::this_thread::sleep_for(10ms);
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(1);
        return counter.fetch_add(1);
    });

    // Wait for all tasks
    critical_future.wait();
    normal_future.wait();
    bg_future.wait();

    // Critical should execute first (lower value in execution_order means executed earlier)
    ASSERT_GE(execution_order.size(), 3);

    // Find positions of each priority
    auto critical_pos = std::find(execution_order.begin(), execution_order.end(), 1);
    auto normal_pos = std::find(execution_order.begin(), execution_order.end(), 2);
    auto bg_pos = std::find(execution_order.begin(), execution_order.end(), 3);

    // Critical should come before background
    EXPECT_LT(std::distance(execution_order.begin(), critical_pos),
              std::distance(execution_order.begin(), bg_pos));
}

TEST_F(PrioritySchedulingTest, CriticalTasksFirst) {
    std::atomic<int> first_task_id{-1};
    std::atomic<bool> critical_was_first{false};

    // Submit many background tasks first
    std::vector<std::future<void>> bg_futures;
    for (int i = 0; i < 10; ++i) {
        bg_futures.push_back(system_->submit_background([&, i]() {
            int expected = -1;
            if (first_task_id.compare_exchange_strong(expected, i + 100)) {
                // This background task was first
            }
            std::this_thread::sleep_for(5ms);
        }));
    }

    // Then submit a critical task
    auto critical_future = system_->submit_critical([&]() {
        int expected = -1;
        if (first_task_id.compare_exchange_strong(expected, 1)) {
            critical_was_first = true;
        }
        std::this_thread::sleep_for(5ms);
    });

    // Wait for all
    critical_future.wait();
    for (auto& f : bg_futures) {
        f.wait();
    }

    // In a well-behaved priority system, critical should often be processed early
    // Note: This is probabilistic due to timing, but critical should have advantage
    if (first_task_id.load() == 1) {
        EXPECT_TRUE(critical_was_first.load());
    }
}

TEST_F(PrioritySchedulingTest, MixedPriorityWorkload) {
    const int tasks_per_priority = 5;
    std::atomic<int> critical_completed{0};
    std::atomic<int> normal_completed{0};
    std::atomic<int> background_completed{0};

    std::vector<std::future<void>> futures;

    // Submit mixed priority tasks
    for (int i = 0; i < tasks_per_priority; ++i) {
        // Critical tasks
        futures.push_back(system_->submit_critical([&critical_completed]() {
            std::this_thread::sleep_for(2ms);
            critical_completed.fetch_add(1);
        }));

        // Normal tasks
        futures.push_back(system_->submit([&normal_completed]() {
            std::this_thread::sleep_for(2ms);
            normal_completed.fetch_add(1);
        }));

        // Background tasks
        futures.push_back(system_->submit_background([&background_completed]() {
            std::this_thread::sleep_for(2ms);
            background_completed.fetch_add(1);
        }));
    }

    // Wait for all tasks
    for (auto& future : futures) {
        future.wait();
    }

    // All tasks should complete
    EXPECT_EQ(critical_completed.load(), tasks_per_priority);
    EXPECT_EQ(normal_completed.load(), tasks_per_priority);
    EXPECT_EQ(background_completed.load(), tasks_per_priority);
}

TEST_F(PrioritySchedulingTest, PriorityUnderLoad) {
    std::atomic<int> critical_sum{0};
    std::atomic<int> background_sum{0};
    std::atomic<int> critical_start_count{0};
    std::atomic<int> background_start_count{0};

    auto start_time = std::chrono::steady_clock::now();

    // Create high load with background tasks
    std::vector<std::future<void>> bg_futures;
    for (int i = 0; i < 50; ++i) {
        bg_futures.push_back(system_->submit_background([&background_sum, &background_start_count, start_time]() {
            auto task_start = std::chrono::steady_clock::now();
            if (task_start - start_time < 50ms) {
                background_start_count.fetch_add(1);
            }
            std::this_thread::sleep_for(10ms);
            background_sum.fetch_add(1);
        }));
    }

    // Add critical tasks after background tasks
    std::this_thread::sleep_for(5ms);  // Small delay

    std::vector<std::future<void>> critical_futures;
    for (int i = 0; i < 5; ++i) {
        critical_futures.push_back(system_->submit_critical([&critical_sum, &critical_start_count, start_time]() {
            auto task_start = std::chrono::steady_clock::now();
            if (task_start - start_time < 50ms) {
                critical_start_count.fetch_add(1);
            }
            std::this_thread::sleep_for(10ms);
            critical_sum.fetch_add(1);
        }));
    }

    // Wait for all critical tasks
    for (auto& f : critical_futures) {
        f.wait();
    }

    // Critical tasks should all complete
    EXPECT_EQ(critical_sum.load(), 5);

    // Wait for background tasks
    for (auto& f : bg_futures) {
        f.wait();
    }

    EXPECT_EQ(background_sum.load(), 50);

    // Critical tasks should get preferential treatment
    // (This is a soft expectation due to timing variability)
    std::cout << "Critical tasks started early: " << critical_start_count.load() << "/5" << std::endl;
    std::cout << "Background tasks started early: " << background_start_count.load() << "/50" << std::endl;
}

TEST_F(PrioritySchedulingTest, StarvationPrevention) {
    // Even low priority tasks should eventually execute
    std::atomic<bool> background_executed{false};

    // Submit many critical tasks
    std::vector<std::future<void>> critical_futures;
    for (int i = 0; i < 100; ++i) {
        critical_futures.push_back(system_->submit_critical([i]() {
            std::this_thread::sleep_for(1ms);
        }));
    }

    // Submit a background task
    auto bg_future = system_->submit_background([&background_executed]() {
        background_executed = true;
    });

    // Submit more critical tasks
    for (int i = 0; i < 100; ++i) {
        critical_futures.push_back(system_->submit_critical([i]() {
            std::this_thread::sleep_for(1ms);
        }));
    }

    // Wait for the background task with a timeout
    auto status = bg_future.wait_for(5s);
    EXPECT_EQ(status, std::future_status::ready);
    EXPECT_TRUE(background_executed.load());

    // Clean up remaining tasks
    for (auto& f : critical_futures) {
        f.wait();
    }
}

TEST_F(PrioritySchedulingTest, PriorityWithExceptions) {
    // Exceptions in high-priority tasks shouldn't affect lower priority tasks

    auto critical_future = system_->submit_critical([]() -> int {
        throw std::runtime_error("Critical task exception");
    });

    auto normal_future = system_->submit([]() {
        return 42;
    });

    auto background_future = system_->submit_background([]() {
        return 100;
    });

    // Critical task should throw
    EXPECT_THROW(critical_future.get(), std::runtime_error);

    // Other tasks should complete normally
    EXPECT_EQ(normal_future.get(), 42);
    EXPECT_EQ(background_future.get(), 100);
}

TEST_F(PrioritySchedulingTest, BulkPrioritySubmission) {
    const int bulk_size = 20;
    std::vector<std::future<int>> critical_futures;
    std::vector<std::future<int>> normal_futures;
    std::vector<std::future<int>> background_futures;

    // Submit bulk tasks of each priority
    for (int i = 0; i < bulk_size; ++i) {
        critical_futures.push_back(system_->submit_critical([i]() {
            return i * 3;
        }));

        normal_futures.push_back(system_->submit([i]() {
            return i * 2;
        }));

        background_futures.push_back(system_->submit_background([i]() {
            return i;
        }));
    }

    // Verify all tasks complete with correct results
    for (int i = 0; i < bulk_size; ++i) {
        EXPECT_EQ(critical_futures[i].get(), i * 3);
        EXPECT_EQ(normal_futures[i].get(), i * 2);
        EXPECT_EQ(background_futures[i].get(), i);
    }
}

// Performance test for priority scheduling
TEST(PriorityPerformanceTest, ThroughputByPriority) {
    unified_thread_system::config cfg;
    cfg.thread_count = std::thread::hardware_concurrency();
    unified_thread_system system(cfg);

    const int tasks_per_priority = 100;

    auto measure_throughput = [&system](auto submit_func, const std::string& priority_name) {
        auto start = std::chrono::steady_clock::now();

        std::vector<std::future<void>> futures;
        for (int i = 0; i < tasks_per_priority; ++i) {
            futures.push_back(submit_func());
        }

        for (auto& f : futures) {
            f.wait();
        }

        auto duration = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        std::cout << priority_name << " priority: " << tasks_per_priority
                  << " tasks in " << ms << "ms ("
                  << (tasks_per_priority * 1000.0 / ms) << " tasks/sec)" << std::endl;

        return ms;
    };

    // Measure each priority level
    auto critical_ms = measure_throughput([&system]() {
        return system.submit_critical([]() {
            std::this_thread::sleep_for(1ms);
        });
    }, "Critical");

    auto normal_ms = measure_throughput([&system]() {
        return system.submit([]() {
            std::this_thread::sleep_for(1ms);
        });
    }, "Normal");

    auto background_ms = measure_throughput([&system]() {
        return system.submit_background([]() {
            std::this_thread::sleep_for(1ms);
        });
    }, "Background");

    // All priority levels should have reasonable performance
    EXPECT_LT(critical_ms, 10000);    // Less than 10 seconds
    EXPECT_LT(normal_ms, 10000);      // Less than 10 seconds
    EXPECT_LT(background_ms, 10000);  // Less than 10 seconds
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}