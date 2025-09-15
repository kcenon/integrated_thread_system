/**
 * @file test_basic_operations.cpp
 * @brief Unit tests for basic thread system operations
 */

#include <gtest/gtest.h>
#include "unified_thread_system.h"
#include <chrono>
#include <vector>
#include <atomic>
#include <numeric>
#include <thread>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

class BasicOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        system_ = std::make_unique<unified_thread_system>();
    }

    void TearDown() override {
        system_.reset();
    }

    std::unique_ptr<unified_thread_system> system_;
};

TEST_F(BasicOperationsTest, SystemCreation) {
    // System should be created successfully
    ASSERT_NE(system_, nullptr);
}

TEST_F(BasicOperationsTest, SimpleTaskSubmission) {
    auto future = system_->submit([]() {
        return 42;
    });

    EXPECT_EQ(future.get(), 42);
}

TEST_F(BasicOperationsTest, MultipleTaskSubmission) {
    const int num_tasks = 10;
    std::vector<std::future<int>> futures;

    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(system_->submit([i]() {
            return i * i;
        }));
    }

    for (int i = 0; i < num_tasks; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

TEST_F(BasicOperationsTest, VoidTaskSubmission) {
    std::atomic<bool> executed{false};

    auto future = system_->submit([&executed]() {
        executed = true;
    });

    future.wait();
    EXPECT_TRUE(executed.load());
}

TEST_F(BasicOperationsTest, TaskWithParameters) {
    int a = 5;
    int b = 10;

    auto future = system_->submit([a, b]() {
        return a + b;
    });

    EXPECT_EQ(future.get(), 15);
}

TEST_F(BasicOperationsTest, TaskWithReferenceCapture) {
    int counter = 0;
    std::mutex counter_mutex;

    std::vector<std::future<void>> futures;
    const int num_increments = 100;

    for (int i = 0; i < num_increments; ++i) {
        futures.push_back(system_->submit([&counter, &counter_mutex]() {
            std::lock_guard<std::mutex> lock(counter_mutex);
            counter++;
        }));
    }

    for (auto& future : futures) {
        future.wait();
    }

    EXPECT_EQ(counter, num_increments);
}

TEST_F(BasicOperationsTest, ExceptionPropagation) {
    auto future = system_->submit([]() -> int {
        throw std::runtime_error("Test exception");
        return 0;  // Never reached
    });

    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST_F(BasicOperationsTest, FutureTimeout) {
    auto future = system_->submit([]() {
        std::this_thread::sleep_for(100ms);
        return 42;
    });

    // Should timeout
    auto status = future.wait_for(10ms);
    EXPECT_EQ(status, std::future_status::timeout);

    // Should be ready after waiting
    status = future.wait_for(200ms);
    EXPECT_EQ(status, std::future_status::ready);

    EXPECT_EQ(future.get(), 42);
}

TEST_F(BasicOperationsTest, ConcurrentTaskExecution) {
    const int num_tasks = 100;
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};

    std::vector<std::future<void>> futures;

    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(system_->submit([&concurrent_count, &max_concurrent]() {
            int current = concurrent_count.fetch_add(1) + 1;

            // Update max concurrent if needed
            int expected = max_concurrent.load();
            while (current > expected &&
                   !max_concurrent.compare_exchange_weak(expected, current)) {
                // Retry
            }

            std::this_thread::sleep_for(10ms);
            concurrent_count.fetch_sub(1);
        }));
    }

    for (auto& future : futures) {
        future.wait();
    }

    // Should have had multiple tasks running concurrently
    EXPECT_GT(max_concurrent.load(), 1);
}

TEST_F(BasicOperationsTest, LargeDataProcessing) {
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 1);

    auto future = system_->submit([data]() {
        return std::accumulate(data.begin(), data.end(), 0LL);
    });

    // Sum of 1 to 10000
    long long expected = 10000LL * 10001LL / 2;
    EXPECT_EQ(future.get(), expected);
}

TEST_F(BasicOperationsTest, MixedReturnTypes) {
    auto int_future = system_->submit([]() { return 42; });
    auto string_future = system_->submit([]() { return std::string("Hello"); });
    auto float_future = system_->submit([]() { return 3.14f; });

    EXPECT_EQ(int_future.get(), 42);
    EXPECT_EQ(string_future.get(), "Hello");
    EXPECT_FLOAT_EQ(float_future.get(), 3.14f);
}

TEST_F(BasicOperationsTest, TaskChaining) {
    auto first = system_->submit([]() {
        return 10;
    });

    int first_result = first.get();

    auto second = system_->submit([first_result]() {
        return first_result * 2;
    });

    int second_result = second.get();

    auto third = system_->submit([second_result]() {
        return second_result + 5;
    });

    EXPECT_EQ(third.get(), 25);  // (10 * 2) + 5
}

// Test with custom configuration
TEST(ConfigurationTest, CustomWorkerCount) {
    unified_thread_system::config cfg;
    cfg.thread_count = 4;

    unified_thread_system system(cfg);

    // Submit enough tasks to utilize all workers
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 8; ++i) {
        futures.push_back(system.submit([i]() {
            std::this_thread::sleep_for(10ms);
            return i;
        }));
    }

    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(futures[i].get(), i);
    }
}

TEST(ConfigurationTest, ZeroConfiguration) {
    // Should work with default configuration
    unified_thread_system system;

    auto future = system.submit([]() {
        return std::string("Zero configuration works!");
    });

    EXPECT_EQ(future.get(), "Zero configuration works!");
}

// Stress test
TEST(StressTest, ManySmallTasks) {
    unified_thread_system system;
    const int num_tasks = 1000;

    std::vector<std::future<int>> futures;
    futures.reserve(num_tasks);

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(system.submit([i]() {
            return i % 100;  // Simple computation
        }));
    }

    for (int i = 0; i < num_tasks; ++i) {
        EXPECT_EQ(futures[i].get(), i % 100);
    }

    auto duration = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    // Should complete reasonably quickly
    EXPECT_LT(ms, 5000);  // Less than 5 seconds for 1000 tasks

    std::cout << "Processed " << num_tasks << " tasks in " << ms << "ms" << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}