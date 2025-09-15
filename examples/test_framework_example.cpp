/**
 * @file test_framework_example.cpp
 * @brief Example demonstrating the integrated test framework
 */

#include <iostream>
#include <kcenon/testing/test_framework.h>
#include <kcenon/testing/test_runner.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/integrated/metrics_aggregator.h>
#include <kcenon/integrated/error_handler.h>

using namespace kcenon::testing;
using namespace kcenon::thread;
using namespace kcenon::integrated;

/**
 * @brief Basic unit test example
 */
class basic_arithmetic_test : public test_case {
public:
    std::string get_name() const override {
        return "BasicArithmetic";
    }
    
    void run() override {
        // Test addition
        TEST_ASSERT_EQ(2 + 2, 4);
        TEST_ASSERT_EQ(10 + 5, 15);
        
        // Test subtraction
        TEST_ASSERT_EQ(10 - 5, 5);
        TEST_ASSERT_EQ(0 - 5, -5);
        
        // Test multiplication
        TEST_ASSERT_EQ(3 * 4, 12);
        TEST_ASSERT_EQ(0 * 100, 0);
        
        // Test division
        TEST_ASSERT_EQ(10 / 2, 5);
        TEST_ASSERT_NE(10 / 3, 3);
        
        // Test floating point
        TEST_ASSERT_NEAR(3.14, 3.14159, 0.01);
    }
};

/**
 * @brief Test with expected failure
 */
class expected_failure_test : public test_case {
public:
    std::string get_name() const override {
        return "ExpectedFailure";
    }
    
    void run() override {
        // This test is designed to fail
        TEST_ASSERT_EQ(1, 2); // Will fail
    }
};

/**
 * @brief Test that should be skipped
 */
class skipped_test : public test_case {
public:
    std::string get_name() const override {
        return "SkippedTest";
    }
    
    bool should_skip() const override {
        return true; // Skip this test
    }
    
    void run() override {
        // This should not run
        TEST_ASSERT(false);
    }
};

/**
 * @brief Thread pool test fixture
 */
struct thread_pool_fixture {
    std::unique_ptr<thread_pool> pool;
    
    thread_pool_fixture() : pool(std::make_unique<thread_pool>(4)) {}
};

/**
 * @brief Thread pool test using fixture
 */
class thread_pool_test : public test_fixture<thread_pool_fixture> {
public:
    std::string get_name() const override {
        return "ThreadPoolOperations";
    }
    
    void run() override {
        auto* fixture = get_fixture();
        TEST_ASSERT(fixture != nullptr);
        TEST_ASSERT(fixture->pool != nullptr);
        
        // Test task submission
        auto future = fixture->pool->submit([]() { return 42; });
        TEST_ASSERT_EQ(future.get(), 42);
        
        // Test multiple tasks
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 10; ++i) {
            futures.push_back(fixture->pool->submit([i]() { 
                return i * i; 
            }));
        }
        
        for (int i = 0; i < 10; ++i) {
            TEST_ASSERT_EQ(futures[i].get(), i * i);
        }
        
        // Test thread count
        TEST_ASSERT_EQ(fixture->pool->get_thread_count(), 4);
    }
};

/**
 * @brief Performance benchmark test
 */
class string_concatenation_benchmark : public benchmark_test {
public:
    string_concatenation_benchmark() 
        : benchmark_test("StringConcatenation") {
        config_.iterations = 10000;
        config_.warmup_iterations = 1000;
    }
    
protected:
    void benchmark_iteration() override {
        std::string result;
        for (int i = 0; i < 100; ++i) {
            result += std::to_string(i);
        }
    }
    
private:
    benchmark_test::config config_;
};

/**
 * @brief Mock example
 */
class mock_logger : public mock_base<logger::logger_interface> {
public:
    void info(std::string_view message) override {
        record_call("info", {std::string(message)});
    }
    
    void warn(std::string_view message) override {
        record_call("warn", {std::string(message)});
    }
    
    void error(std::string_view message) override {
        record_call("error", {std::string(message)});
    }
    
    void critical(std::string_view message) override {
        record_call("critical", {std::string(message)});
    }
};

/**
 * @brief Test using mock object
 */
class mock_test : public test_case {
public:
    std::string get_name() const override {
        return "MockLogger";
    }
    
    void run() override {
        mock_logger logger;
        
        // Use the mock
        logger.info("Test message");
        logger.warn("Warning message");
        logger.error("Error message");
        
        // Verify calls
        TEST_ASSERT(logger.was_called("info"));
        TEST_ASSERT(logger.was_called("warn"));
        TEST_ASSERT(logger.was_called("error"));
        TEST_ASSERT_FALSE(logger.was_called("critical"));
        
        // Verify call counts
        TEST_ASSERT_EQ(logger.get_call_count("info"), 1);
        TEST_ASSERT_EQ(logger.get_call_count("warn"), 1);
        TEST_ASSERT_EQ(logger.get_call_count("error"), 1);
        TEST_ASSERT_EQ(logger.get_call_count("critical"), 0);
    }
};

/**
 * @brief Integration test for metrics aggregator
 */
class metrics_integration_test : public test_case {
public:
    std::string get_name() const override {
        return "MetricsAggregatorIntegration";
    }
    
    std::chrono::milliseconds get_timeout() const override {
        return std::chrono::milliseconds(10000); // 10 seconds
    }
    
    void run() override {
        // Create metrics aggregator
        metrics_aggregator::config config;
        config.collection_interval = std::chrono::milliseconds(100);
        metrics_aggregator aggregator(config);
        
        // Start collection
        TEST_ASSERT(aggregator.start());
        TEST_ASSERT(aggregator.is_running());
        
        // Wait for some metrics
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Get current metrics
        auto metrics = aggregator.get_current_metrics();
        TEST_ASSERT(metrics.timestamp != std::chrono::system_clock::time_point{});
        
        // Stop collection
        aggregator.stop();
        TEST_ASSERT_FALSE(aggregator.is_running());
        
        // Get history
        auto history = aggregator.get_history(std::chrono::seconds(1));
        TEST_ASSERT(history.size() > 0);
    }
};

/**
 * @brief Test with exception handling
 */
class exception_test : public test_case {
public:
    std::string get_name() const override {
        return "ExceptionHandling";
    }
    
    void run() override {
        // Test that exception is thrown
        TEST_ASSERT_THROWS(throw std::runtime_error("Test error"));
        
        // Test that no exception is thrown
        TEST_ASSERT_NO_THROW(int x = 5 + 5);
        
        // Test specific exception handling
        try {
            throw std::invalid_argument("Invalid argument");
            TEST_ASSERT(false); // Should not reach here
        } catch (const std::invalid_argument& e) {
            TEST_ASSERT_EQ(std::string(e.what()), "Invalid argument");
        }
    }
};

/**
 * @brief Timeout test
 */
class timeout_test : public test_case {
public:
    std::string get_name() const override {
        return "TimeoutTest";
    }
    
    std::chrono::milliseconds get_timeout() const override {
        return std::chrono::milliseconds(100); // 100ms timeout
    }
    
    void run() override {
        // This will timeout
        std::this_thread::sleep_for(std::chrono::seconds(1));
        TEST_ASSERT(false); // Should not reach here
    }
};

int main() {
    std::cout << "=== Test Framework Example ===" << std::endl;
    
    // Create test runner
    test_runner_config config;
    config.parallel_execution = true;
    config.max_parallel_tests = 4;
    config.stop_on_failure = false;
    config.verbose = true;
    config.generate_report = true;
    config.report_path = "test_results.txt";
    
    test_runner runner(config);
    
    // Register test observer
    runner.register_observer([](const test_result& result) {
        if (result.status == test_status::failed) {
            std::cout << "    [Observer] Test failed: " 
                     << result.test_name << std::endl;
        }
    });
    
    // Create test suites
    auto basic_suite = std::make_unique<test_suite>("BasicTests");
    basic_suite->add_test(std::make_unique<basic_arithmetic_test>());
    basic_suite->add_test(std::make_unique<expected_failure_test>());
    basic_suite->add_test(std::make_unique<skipped_test>());
    basic_suite->add_test(std::make_unique<exception_test>());
    
    auto thread_suite = std::make_unique<test_suite>("ThreadTests");
    thread_suite->add_test(std::make_unique<thread_pool_test>());
    thread_suite->add_test(std::make_unique<timeout_test>());
    
    auto integration_suite = std::make_unique<test_suite>("IntegrationTests");
    integration_suite->add_test(std::make_unique<metrics_integration_test>());
    integration_suite->add_test(std::make_unique<mock_test>());
    
    auto benchmark_suite = std::make_unique<test_suite>("BenchmarkTests");
    benchmark_suite->add_test(std::make_unique<string_concatenation_benchmark>());
    
    // Add suites to runner
    runner.add_suite(std::move(basic_suite));
    runner.add_suite(std::move(thread_suite));
    runner.add_suite(std::move(integration_suite));
    runner.add_suite(std::move(benchmark_suite));
    
    // Run all tests
    bool success = runner.run();
    
    // Get results for analysis
    const auto& results = runner.get_results();
    
    // Example: Print performance metrics for benchmarks
    std::cout << "\nBenchmark Results:" << std::endl;
    for (const auto& result : results) {
        if (result.suite_name == "BenchmarkTests" && 
            result.status == test_status::passed) {
            std::cout << "  " << result.test_name << ":" << std::endl;
            for (const auto& [metric, value] : result.performance_metrics) {
                std::cout << "    " << metric << ": " << value << std::endl;
            }
        }
    }
    
    // Return exit code based on test results
    return success ? 0 : 1;
}
