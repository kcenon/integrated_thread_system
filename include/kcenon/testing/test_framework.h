#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <kcenon/thread/core/event_bus.h>
#include <kcenon/integrated/metrics_aggregator.h>
#include <kcenon/integrated/error_handler.h>

namespace kcenon::testing {

// Note: Fully qualify types from integrated:: and thread:: namespaces
// instead of using 'using namespace' directives

/**
 * @brief Test result status
 */
enum class test_status {
    not_run,
    running,
    passed,
    failed,
    skipped,
    timeout
};

/**
 * @brief Test result information
 */
struct test_result {
    std::string test_name;
    std::string suite_name;
    test_status status{test_status::not_run};
    std::chrono::nanoseconds duration{0};
    std::string failure_message;
    std::string stack_trace;
    std::unordered_map<std::string, double> performance_metrics;
    
    bool is_success() const {
        return status == test_status::passed || status == test_status::skipped;
    }
};

/**
 * @brief Test case interface
 */
class test_case {
public:
    virtual ~test_case() = default;
    
    /**
     * @brief Get test name
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Setup before test
     */
    virtual void setup() {}
    
    /**
     * @brief Run the test
     */
    virtual void run() = 0;
    
    /**
     * @brief Teardown after test
     */
    virtual void teardown() {}
    
    /**
     * @brief Check if test should be skipped
     */
    virtual bool should_skip() const { return false; }
    
    /**
     * @brief Get test timeout
     */
    virtual std::chrono::milliseconds get_timeout() const {
        return std::chrono::milliseconds(5000); // 5 seconds default
    }
};

/**
 * @brief Test suite for grouping related tests
 */
class test_suite {
public:
    explicit test_suite(const std::string& name)
        : name_(name) {}
    
    /**
     * @brief Add a test case
     */
    void add_test(std::unique_ptr<test_case> test) {
        tests_.push_back(std::move(test));
    }
    
    /**
     * @brief Get suite name
     */
    const std::string& get_name() const { return name_; }
    
    /**
     * @brief Get all tests
     */
    const std::vector<std::unique_ptr<test_case>>& get_tests() const {
        return tests_;
    }
    
    /**
     * @brief Setup before suite
     */
    virtual void suite_setup() {}
    
    /**
     * @brief Teardown after suite
     */
    virtual void suite_teardown() {}
    
private:
    std::string name_;
    std::vector<std::unique_ptr<test_case>> tests_;
};

/**
 * @brief Test fixture for common test setup
 */
template<typename T>
class test_fixture : public test_case {
public:
    void setup() override {
        fixture_ = std::make_unique<T>();
        fixture_setup();
    }
    
    void teardown() override {
        fixture_teardown();
        fixture_.reset();
    }
    
protected:
    virtual void fixture_setup() {}
    virtual void fixture_teardown() {}
    
    T* get_fixture() { return fixture_.get(); }
    const T* get_fixture() const { return fixture_.get(); }
    
private:
    std::unique_ptr<T> fixture_;
};

/**
 * @brief Performance benchmark test
 */
class benchmark_test : public test_case {
public:
    struct config {
        std::size_t iterations{1000};
        std::size_t warmup_iterations{100};
        std::chrono::seconds max_duration{30};
        bool collect_memory_stats{true};
    };
    
    explicit benchmark_test(const std::string& name, const config& cfg = {})
        : name_(name), config_(cfg) {}
    
    std::string get_name() const override { return name_; }
    
    void run() override {
        // Warmup
        for (std::size_t i = 0; i < config_.warmup_iterations; ++i) {
            benchmark_iteration();
        }
        
        // Actual benchmark
        std::vector<std::chrono::nanoseconds> timings;
        auto start_time = std::chrono::steady_clock::now();
        
        for (std::size_t i = 0; i < config_.iterations; ++i) {
            auto iter_start = std::chrono::high_resolution_clock::now();
            benchmark_iteration();
            auto iter_end = std::chrono::high_resolution_clock::now();
            
            timings.push_back(iter_end - iter_start);
            
            // Check timeout
            if (std::chrono::steady_clock::now() - start_time > config_.max_duration) {
                break;
            }
        }
        
        // Calculate statistics
        calculate_statistics(timings);
    }
    
protected:
    /**
     * @brief Single benchmark iteration to measure
     */
    virtual void benchmark_iteration() = 0;
    
    /**
     * @brief Get benchmark results
     */
    const std::unordered_map<std::string, double>& get_results() const {
        return results_;
    }
    
private:
    void calculate_statistics(const std::vector<std::chrono::nanoseconds>& timings) {
        if (timings.empty()) return;
        
        // Sort for percentiles
        std::vector<std::chrono::nanoseconds> sorted_timings = timings;
        std::sort(sorted_timings.begin(), sorted_timings.end());
        
        // Calculate mean
        auto sum = std::chrono::nanoseconds(0);
        for (const auto& t : timings) {
            sum += t;
        }
        double mean_ns = static_cast<double>(sum.count()) / timings.size();
        
        // Calculate percentiles
        auto p50_idx = timings.size() * 50 / 100;
        auto p95_idx = timings.size() * 95 / 100;
        auto p99_idx = timings.size() * 99 / 100;
        
        results_["mean_ms"] = mean_ns / 1e6;
        results_["p50_ms"] = sorted_timings[p50_idx].count() / 1e6;
        results_["p95_ms"] = sorted_timings[p95_idx].count() / 1e6;
        results_["p99_ms"] = sorted_timings[p99_idx].count() / 1e6;
        results_["min_ms"] = sorted_timings.front().count() / 1e6;
        results_["max_ms"] = sorted_timings.back().count() / 1e6;
        results_["iterations"] = static_cast<double>(timings.size());
        
        // Calculate throughput
        double total_seconds = sum.count() / 1e9;
        results_["throughput_ops"] = timings.size() / total_seconds;
    }
    
    std::string name_;
    config config_;
    std::unordered_map<std::string, double> results_;
};

/**
 * @brief Load test for stress testing
 */
class load_test : public test_case {
public:
    struct config {
        std::size_t concurrent_users{10};
        std::chrono::seconds duration{60};
        std::size_t ramp_up_seconds{10};
        double target_rps{100.0}; // Requests per second
    };
    
    explicit load_test(const std::string& name, const config& cfg = {})
        : name_(name), config_(cfg) {}
    
    std::string get_name() const override { return name_; }
    
    std::chrono::milliseconds get_timeout() const override {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            config_.duration + std::chrono::seconds(30) // Extra time for cleanup
        );
    }
    
protected:
    /**
     * @brief Single user simulation
     */
    virtual void simulate_user() = 0;
    
    /**
     * @brief Check system health during load
     */
    virtual bool is_system_healthy() { return true; }
    
private:
    std::string name_;
    config config_;
};

/**
 * @brief Mock object base for testing
 */
template<typename Interface>
class mock_base : public Interface {
public:
    /**
     * @brief Record a method call
     */
    void record_call(const std::string& method_name,
                    const std::vector<std::any>& args = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        call_history_.push_back({method_name, args});
        call_counts_[method_name]++;
    }
    
    /**
     * @brief Get call count for a method
     */
    std::size_t get_call_count(const std::string& method_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = call_counts_.find(method_name);
        return it != call_counts_.end() ? it->second : 0;
    }
    
    /**
     * @brief Verify method was called
     */
    bool was_called(const std::string& method_name) const {
        return get_call_count(method_name) > 0;
    }
    
    /**
     * @brief Reset mock state
     */
    void reset_mock() {
        std::lock_guard<std::mutex> lock(mutex_);
        call_history_.clear();
        call_counts_.clear();
    }
    
private:
    mutable std::mutex mutex_;
    std::vector<std::pair<std::string, std::vector<std::any>>> call_history_;
    std::unordered_map<std::string, std::size_t> call_counts_;
};

/**
 * @brief Test assertion utilities
 */
class test_assert {
public:
    static void assert_true(bool condition, const std::string& message = "") {
        if (!condition) {
            throw std::runtime_error("Assertion failed: " + 
                (message.empty() ? "expected true" : message));
        }
    }
    
    static void assert_false(bool condition, const std::string& message = "") {
        assert_true(!condition, message.empty() ? "expected false" : message);
    }
    
    template<typename T>
    static void assert_equal(const T& expected, const T& actual,
                            const std::string& message = "") {
        if (expected != actual) {
            std::stringstream ss;
            ss << "Assertion failed: expected '" << expected 
               << "' but got '" << actual << "'";
            if (!message.empty()) {
                ss << " - " << message;
            }
            throw std::runtime_error(ss.str());
        }
    }
    
    template<typename T>
    static void assert_not_equal(const T& not_expected, const T& actual,
                                const std::string& message = "") {
        if (not_expected == actual) {
            std::stringstream ss;
            ss << "Assertion failed: did not expect '" << not_expected << "'";
            if (!message.empty()) {
                ss << " - " << message;
            }
            throw std::runtime_error(ss.str());
        }
    }
    
    static void assert_throws(std::function<void()> func,
                            const std::string& message = "") {
        bool threw = false;
        try {
            func();
        } catch (...) {
            threw = true;
        }
        assert_true(threw, message.empty() ? "expected exception" : message);
    }
    
    static void assert_no_throw(std::function<void()> func,
                               const std::string& message = "") {
        try {
            func();
        } catch (const std::exception& e) {
            std::string msg = message.empty() ? 
                "unexpected exception: " : message + ": ";
            throw std::runtime_error(msg + e.what());
        }
    }
    
    template<typename T>
    static void assert_near(const T& expected, const T& actual, const T& tolerance,
                           const std::string& message = "") {
        T diff = std::abs(expected - actual);
        if (diff > tolerance) {
            std::stringstream ss;
            ss << "Assertion failed: expected '" << expected 
               << "' ± " << tolerance << " but got '" << actual << "'";
            if (!message.empty()) {
                ss << " - " << message;
            }
            throw std::runtime_error(ss.str());
        }
    }
};

// Convenience macros
#define TEST_ASSERT(condition) \
    kcenon::testing::test_assert::assert_true(condition, #condition)

#define TEST_ASSERT_TRUE(condition) \
    kcenon::testing::test_assert::assert_true(condition)

#define TEST_ASSERT_FALSE(condition) \
    kcenon::testing::test_assert::assert_false(condition)

#define TEST_ASSERT_EQ(expected, actual) \
    kcenon::testing::test_assert::assert_equal(expected, actual)

#define TEST_ASSERT_NE(not_expected, actual) \
    kcenon::testing::test_assert::assert_not_equal(not_expected, actual)

#define TEST_ASSERT_NEAR(expected, actual, tolerance) \
    kcenon::testing::test_assert::assert_near(expected, actual, tolerance)

#define TEST_ASSERT_THROWS(expr) \
    kcenon::testing::test_assert::assert_throws([&](){ expr; })

#define TEST_ASSERT_NO_THROW(expr) \
    kcenon::testing::test_assert::assert_no_throw([&](){ expr; })

} // namespace kcenon::testing