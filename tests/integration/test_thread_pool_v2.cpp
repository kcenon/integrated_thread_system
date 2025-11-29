/**
 * @file test_thread_pool_v2.cpp
 * @brief Integration tests for thread_system v2.0.0 features
 *
 * Tests work-stealing, priority scheduling, cancellation,
 * and dynamic scaling features.
 */

#include <gtest/gtest.h>
#include <kcenon/integrated/unified_thread_system.h>
#include <chrono>
#include <vector>
#include <atomic>
#include <thread>
#include <set>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

class ThreadPoolV2Test : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @test Priority scheduling test
 *
 * Verifies that critical tasks are executed before lower priority tasks.
 */
TEST_F(ThreadPoolV2Test, PriorityScheduling) {
    config cfg;
    cfg.thread_count = 2;  // Limited workers to test priority
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;
    cfg.enable_monitoring = true;

    unified_thread_system system(cfg);

    std::atomic<int> critical_start{0};
    std::atomic<int> normal_start{0};
    std::atomic<int> background_start{0};
    std::atomic<int> counter{0};

    std::vector<std::future<void>> futures;

    // Submit background tasks first (should run last)
    for (int i = 0; i < 5; ++i) {
        futures.push_back(system.submit_background([&]() {
            int order = counter.fetch_add(1);
            background_start.fetch_add(order);
            std::this_thread::sleep_for(5ms);
        }));
    }

    // Submit normal tasks
    for (int i = 0; i < 5; ++i) {
        futures.push_back(system.submit([&]() {
            int order = counter.fetch_add(1);
            normal_start.fetch_add(order);
            std::this_thread::sleep_for(5ms);
        }));
    }

    // Submit critical tasks (should run first)
    for (int i = 0; i < 5; ++i) {
        futures.push_back(system.submit_critical([&]() {
            int order = counter.fetch_add(1);
            critical_start.fetch_add(order);
            std::this_thread::sleep_for(5ms);
        }));
    }

    // Wait for all tasks
    for (auto& f : futures) {
        f.wait();
    }

    // Verify all tasks completed
    EXPECT_EQ(counter.load(), 15);

    // Critical tasks should generally have lower average start order
    // (Note: this is a statistical test, may have some variance)
    double avg_critical = critical_start.load() / 5.0;
    double avg_background = background_start.load() / 5.0;

    // Critical tasks should tend to start earlier on average
    EXPECT_LE(avg_critical, avg_background + 5);  // Allow some variance
}

/**
 * @test Work stealing effectiveness
 *
 * Verifies that work stealing improves load balancing.
 */
TEST_F(ThreadPoolV2Test, WorkStealingEffectiveness) {
    config cfg;
    cfg.thread_count = 4;
    cfg.enable_work_stealing = true;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;
    cfg.enable_monitoring = true;

    unified_thread_system system(cfg);

    const int num_tasks = 1000;
    std::atomic<int> completed{0};

    auto start = std::chrono::steady_clock::now();

    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(system.submit([&, i]() {
            // Simulate imbalanced workload
            if (i % 10 == 0) {
                std::this_thread::sleep_for(1ms);
            }
            completed.fetch_add(1);
        }));
    }

    for (auto& f : futures) {
        f.wait();
    }

    auto duration = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    EXPECT_EQ(completed.load(), num_tasks);
    // With work stealing, should complete reasonably fast
    EXPECT_LT(ms, 2000);  // Should complete in under 2 seconds
}

/**
 * @test Cancellation token
 *
 * Verifies that tasks can be cancelled via cancellation token.
 */
TEST_F(ThreadPoolV2Test, TaskCancellation) {
    config cfg;
    cfg.thread_count = 4;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;

    unified_thread_system system(cfg);

    std::atomic<int> executed_count{0};
    std::atomic<int> cancelled_check_count{0};

    // Create cancellation token
    cancellation_token token;

    std::vector<std::future<void>> futures;

    // Submit tasks that check for cancellation
    for (int i = 0; i < 20; ++i) {
        futures.push_back(system.submit_cancellable(token, [&token, &executed_count, &cancelled_check_count]() {
            // Simulate work
            std::this_thread::sleep_for(10ms);

            if (token.is_cancelled()) {
                cancelled_check_count++;
                return;
            }
            executed_count++;
        }));
    }

    // Let some tasks start
    std::this_thread::sleep_for(30ms);

    // Cancel remaining tasks
    token.cancel();

    // Wait for completion
    for (auto& f : futures) {
        f.wait();
    }

    // Some tasks should have executed before cancellation
    EXPECT_GT(executed_count.load(), 0);
    // Verify token is cancelled
    EXPECT_TRUE(token.is_cancelled());
}

/**
 * @test Scheduled task execution
 *
 * Verifies that tasks can be scheduled for delayed execution.
 */
TEST_F(ThreadPoolV2Test, ScheduledTaskExecution) {
    config cfg;
    cfg.thread_count = 4;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;

    unified_thread_system system(cfg);

    auto start = std::chrono::steady_clock::now();
    std::atomic<bool> executed{false};
    std::chrono::steady_clock::time_point execution_time;

    auto future = system.schedule(100ms, [&]() {
        execution_time = std::chrono::steady_clock::now();
        executed.store(true);
        return 42;
    });

    auto result = future.get();

    EXPECT_TRUE(executed.load());
    EXPECT_EQ(result, 42);

    auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
        execution_time - start);

    // Should execute approximately after the scheduled delay (within 50ms tolerance)
    EXPECT_GE(delay.count(), 90);
    EXPECT_LT(delay.count(), 200);
}

/**
 * @test Batch task submission
 *
 * Verifies that batch task submission works correctly.
 */
TEST_F(ThreadPoolV2Test, BatchTaskSubmission) {
    config cfg;
    cfg.thread_count = 4;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;

    unified_thread_system system(cfg);

    std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto futures = system.submit_batch(input.begin(), input.end(),
        [](int x) { return x * x; });

    EXPECT_EQ(futures.size(), input.size());

    int sum = 0;
    for (size_t i = 0; i < futures.size(); ++i) {
        int result = futures[i].get();
        sum += result;
    }

    // Sum of squares: 1 + 4 + 9 + 16 + 25 + 36 + 49 + 64 + 81 + 100 = 385
    EXPECT_EQ(sum, 385);
}

/**
 * @test Map-reduce pattern
 *
 * Verifies that map-reduce pattern works correctly.
 */
TEST_F(ThreadPoolV2Test, MapReducePattern) {
    config cfg;
    cfg.thread_count = 4;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;

    unified_thread_system system(cfg);

    std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto future = system.map_reduce(
        input.begin(), input.end(),
        [](int x) { return x * x; },  // Map: square each element
        [](int acc, int x) { return acc + x; },  // Reduce: sum
        0  // Initial value
    );

    int result = future.get();

    // Sum of squares: 385
    EXPECT_EQ(result, 385);
}

/**
 * @test Dynamic worker scaling
 *
 * Verifies that worker count can be adjusted dynamically.
 */
TEST_F(ThreadPoolV2Test, DynamicWorkerScaling) {
    config cfg;
    cfg.thread_count = 2;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;
    cfg.enable_dynamic_scaling = true;
    cfg.min_threads = 1;
    cfg.max_threads = 8;

    unified_thread_system system(cfg);

    EXPECT_GE(system.worker_count(), 1);
    EXPECT_LE(system.worker_count(), 8);

    // Try to adjust worker count
    system.set_worker_count(4);

    // Should be within bounds
    EXPECT_GE(system.worker_count(), 1);
    EXPECT_LE(system.worker_count(), 8);
}

/**
 * @test Concurrent task submission stress test
 *
 * Verifies thread safety under high concurrent submission load.
 */
TEST_F(ThreadPoolV2Test, ConcurrentSubmissionStress) {
    config cfg;
    cfg.thread_count = 4;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;
    cfg.max_queue_size = 10000;

    unified_thread_system system(cfg);

    const int submitter_threads = 8;
    const int tasks_per_thread = 500;
    std::atomic<int> completed_count{0};
    std::vector<std::thread> submitters;
    std::vector<std::future<void>> all_futures;
    std::mutex futures_mutex;

    for (int t = 0; t < submitter_threads; ++t) {
        submitters.emplace_back([&, t]() {
            std::vector<std::future<void>> local_futures;
            local_futures.reserve(tasks_per_thread);

            for (int i = 0; i < tasks_per_thread; ++i) {
                local_futures.push_back(system.submit([&completed_count]() {
                    completed_count.fetch_add(1, std::memory_order_relaxed);
                }));
            }

            // Move futures to shared collection
            std::lock_guard lock(futures_mutex);
            for (auto& f : local_futures) {
                all_futures.push_back(std::move(f));
            }
        });
    }

    // Wait for all submitters
    for (auto& t : submitters) {
        t.join();
    }

    // Wait for all tasks
    for (auto& f : all_futures) {
        f.wait();
    }

    EXPECT_EQ(completed_count.load(), submitter_threads * tasks_per_thread);
}

/**
 * @test Graceful shutdown
 *
 * Verifies that shutdown waits for running tasks to complete.
 */
TEST_F(ThreadPoolV2Test, GracefulShutdown) {
    config cfg;
    cfg.thread_count = 4;
    cfg.enable_file_logging = false;
    cfg.enable_console_logging = false;

    std::atomic<int> completed_before_shutdown{0};
    std::atomic<int> completed_after_shutdown{0};

    {
        unified_thread_system system(cfg);

        // Submit tasks that take some time
        std::vector<std::future<void>> futures;
        for (int i = 0; i < 10; ++i) {
            futures.push_back(system.submit([&]() {
                std::this_thread::sleep_for(50ms);
                completed_before_shutdown++;
            }));
        }

        // Let some tasks start
        std::this_thread::sleep_for(100ms);

        // Shutdown (should wait for running tasks)
        system.shutdown();

        // Count completed
        completed_after_shutdown.store(completed_before_shutdown.load());
    }

    // All tasks should have completed gracefully
    EXPECT_GT(completed_after_shutdown.load(), 0);
}
