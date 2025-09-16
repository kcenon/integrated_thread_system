#pragma once

#include <kcenon/testing/test_framework.h>
#include <kcenon/thread/core/thread_pool.h>
#include <future>
#include <fstream>
#include <iomanip>

namespace kcenon::testing {

/**
 * @brief Test execution event
 */
struct test_execution_event : event_base {
    test_result result;
    
    explicit test_execution_event(test_result res)
        : result(std::move(res)) {}
    
    std::string type_name() const override {
        return "TestExecutionEvent";
    }
};

/**
 * @brief Test runner configuration
 */
struct test_runner_config {
    bool parallel_execution{true};
    std::size_t max_parallel_tests{4};
    bool stop_on_failure{false};
    bool verbose{true};
    bool generate_report{true};
    std::string report_path{"test_report.txt"};
    bool collect_coverage{false};
    std::chrono::seconds global_timeout{300}; // 5 minutes
};

/**
 * @brief Test runner for executing test suites
 */
class test_runner {
public:
    /**
     * @brief Constructor
     * @param config Runner configuration
     * @param bus Event bus for test events
     */
    explicit test_runner(const test_runner_config& config = {},
                        std::shared_ptr<event_bus> bus = nullptr)
        : config_(config), event_bus_(bus) {
        if (!event_bus_) {
            event_bus_ = std::make_shared<event_bus>();
        }
        
        if (config_.parallel_execution) {
            thread_pool_ = std::make_shared<thread_pool>(config_.max_parallel_tests);
        }
    }
    
    /**
     * @brief Add a test suite
     */
    void add_suite(std::unique_ptr<test_suite> suite) {
        suites_.push_back(std::move(suite));
    }
    
    /**
     * @brief Run all test suites
     * @return True if all tests passed
     */
    bool run() {
        auto start_time = std::chrono::steady_clock::now();
        
        std::cout << "\n" << get_separator() << std::endl;
        std::cout << "Running " << count_total_tests() << " tests from " 
                 << suites_.size() << " test suites" << std::endl;
        std::cout << get_separator() << std::endl;
        
        bool all_passed = true;
        
        for (auto& suite : suites_) {
            if (!run_suite(*suite)) {
                all_passed = false;
                if (config_.stop_on_failure) {
                    break;
                }
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto total_duration = end_time - start_time;
        
        // Print summary
        print_summary(total_duration);
        
        // Generate report if configured
        if (config_.generate_report) {
            generate_report();
        }
        
        return all_passed;
    }
    
    /**
     * @brief Get test results
     */
    const std::vector<test_result>& get_results() const {
        return results_;
    }
    
    /**
     * @brief Register test observer
     */
    void register_observer(std::function<void(const test_result&)> observer) {
        observers_.push_back(std::move(observer));
    }
    
private:
    /**
     * @brief Run a single test suite
     */
    bool run_suite(test_suite& suite) {
        std::cout << "\n[Suite] " << suite.get_name() << std::endl;
        
        // Suite setup
        try {
            suite.suite_setup();
        } catch (const std::exception& e) {
            std::cerr << "  Suite setup failed: " << e.what() << std::endl;
            return false;
        }
        
        bool suite_passed = true;
        std::vector<std::future<test_result>> futures;
        
        for (auto& test : suite.get_tests()) {
            if (config_.parallel_execution && thread_pool_) {
                // Run test asynchronously
                futures.push_back(thread_pool_->submit(
                    [this, &suite, &test]() {
                        return run_test(suite.get_name(), *test);
                    }
                ));
            } else {
                // Run test synchronously
                auto result = run_test(suite.get_name(), *test);
                process_result(result);
                if (!result.is_success()) {
                    suite_passed = false;
                    if (config_.stop_on_failure) {
                        break;
                    }
                }
            }
        }
        
        // Wait for async tests
        for (auto& future : futures) {
            auto result = future.get();
            process_result(result);
            if (!result.is_success()) {
                suite_passed = false;
            }
        }
        
        // Suite teardown
        try {
            suite.suite_teardown();
        } catch (const std::exception& e) {
            std::cerr << "  Suite teardown failed: " << e.what() << std::endl;
        }
        
        return suite_passed;
    }
    
    /**
     * @brief Run a single test case
     */
    test_result run_test(const std::string& suite_name, test_case& test) {
        test_result result;
        result.suite_name = suite_name;
        result.test_name = test.get_name();
        
        // Check if should skip
        if (test.should_skip()) {
            result.status = test_status::skipped;
            return result;
        }
        
        result.status = test_status::running;
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Setup
            test.setup();
            
            // Run with timeout
            auto timeout = test.get_timeout();
            std::promise<void> test_promise;
            auto test_future = test_promise.get_future();
            
            std::thread test_thread([&test, &test_promise]() {
                try {
                    test.run();
                    test_promise.set_value();
                } catch (...) {
                    test_promise.set_exception(std::current_exception());
                }
            });
            
            if (test_future.wait_for(timeout) == std::future_status::timeout) {
                result.status = test_status::timeout;
                result.failure_message = "Test exceeded timeout of " + 
                    std::to_string(timeout.count()) + "ms";
                test_thread.detach(); // Let it finish in background
            } else {
                test_thread.join();
                try {
                    test_future.get();
                    result.status = test_status::passed;
                } catch (const std::exception& e) {
                    result.status = test_status::failed;
                    result.failure_message = e.what();
                }
            }
            
            // Teardown
            test.teardown();
            
        } catch (const std::exception& e) {
            result.status = test_status::failed;
            result.failure_message = "Setup/Teardown error: ";
            result.failure_message += e.what();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = end - start;
        
        return result;
    }
    
    /**
     * @brief Process test result
     */
    void process_result(const test_result& result) {
        {
            std::lock_guard<std::mutex> lock(results_mutex_);
            results_.push_back(result);
            
            // Update statistics
            switch (result.status) {
                case test_status::passed:
                    passed_count_++;
                    break;
                case test_status::failed:
                    failed_count_++;
                    break;
                case test_status::skipped:
                    skipped_count_++;
                    break;
                case test_status::timeout:
                    timeout_count_++;
                    break;
                default:
                    break;
            }
        }
        
        // Print result
        print_test_result(result);
        
        // Notify observers
        for (const auto& observer : observers_) {
            observer(result);
        }
        
        // Publish event
        if (event_bus_) {
            event_bus_->publish(test_execution_event(result));
        }
    }
    
    /**
     * @brief Print test result
     */
    void print_test_result(const test_result& result) {
        std::lock_guard<std::mutex> lock(print_mutex_);
        
        std::cout << "  ";
        
        // Status indicator
        switch (result.status) {
            case test_status::passed:
                std::cout << "[\033[32mPASS\033[0m]";
                break;
            case test_status::failed:
                std::cout << "[\033[31mFAIL\033[0m]";
                break;
            case test_status::skipped:
                std::cout << "[\033[33mSKIP\033[0m]";
                break;
            case test_status::timeout:
                std::cout << "[\033[35mTIME\033[0m]";
                break;
            default:
                std::cout << "[????]";
        }
        
        std::cout << " " << result.test_name;
        
        // Duration
        if (result.status == test_status::passed || result.status == test_status::failed) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(result.duration);
            std::cout << " (" << ms.count() << " ms)";
        }
        
        std::cout << std::endl;
        
        // Failure details
        if (config_.verbose && !result.failure_message.empty()) {
            std::cout << "      Error: " << result.failure_message << std::endl;
        }
    }
    
    /**
     * @brief Print summary
     */
    void print_summary(std::chrono::nanoseconds total_duration) {
        std::cout << "\n" << get_separator() << std::endl;
        std::cout << "Test Summary:" << std::endl;
        std::cout << get_separator() << std::endl;
        
        auto total = passed_count_ + failed_count_ + skipped_count_ + timeout_count_;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_duration);
        
        std::cout << "Total tests: " << total << std::endl;
        std::cout << "  Passed:  \033[32m" << passed_count_ << "\033[0m" << std::endl;
        std::cout << "  Failed:  \033[31m" << failed_count_ << "\033[0m" << std::endl;
        std::cout << "  Skipped: \033[33m" << skipped_count_ << "\033[0m" << std::endl;
        std::cout << "  Timeout: \033[35m" << timeout_count_ << "\033[0m" << std::endl;
        std::cout << "\nTotal time: " << ms.count() << " ms" << std::endl;
        
        // Result
        if (failed_count_ == 0 && timeout_count_ == 0) {
            std::cout << "\n\033[32m✓ All tests passed!\033[0m" << std::endl;
        } else {
            std::cout << "\n\033[31m✗ Some tests failed!\033[0m" << std::endl;
            
            // List failed tests
            std::cout << "\nFailed tests:" << std::endl;
            for (const auto& result : results_) {
                if (result.status == test_status::failed || 
                    result.status == test_status::timeout) {
                    std::cout << "  - " << result.suite_name << "::" 
                             << result.test_name << std::endl;
                }
            }
        }
        
        std::cout << get_separator() << std::endl;
    }
    
    /**
     * @brief Generate test report
     */
    void generate_report() {
        std::ofstream report(config_.report_path);
        if (!report.is_open()) {
            std::cerr << "Failed to create report file: " << config_.report_path << std::endl;
            return;
        }
        
        report << "Test Execution Report" << std::endl;
        report << "===================" << std::endl;
        report << "Date: " << get_current_time() << std::endl;
        report << std::endl;
        
        // Summary
        report << "Summary:" << std::endl;
        report << "--------" << std::endl;
        report << "Total: " << results_.size() << std::endl;
        report << "Passed: " << passed_count_ << std::endl;
        report << "Failed: " << failed_count_ << std::endl;
        report << "Skipped: " << skipped_count_ << std::endl;
        report << "Timeout: " << timeout_count_ << std::endl;
        report << std::endl;
        
        // Detailed results
        report << "Detailed Results:" << std::endl;
        report << "----------------" << std::endl;
        
        std::string current_suite;
        for (const auto& result : results_) {
            if (result.suite_name != current_suite) {
                current_suite = result.suite_name;
                report << std::endl << "[" << current_suite << "]" << std::endl;
            }
            
            report << "  " << std::setw(8) << status_to_string(result.status)
                  << " | " << result.test_name;
            
            if (result.duration.count() > 0) {
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(result.duration);
                report << " | " << ms.count() << " ms";
            }
            
            if (!result.failure_message.empty()) {
                report << std::endl << "         Error: " << result.failure_message;
            }
            
            report << std::endl;
        }
        
        report.close();
        std::cout << "\nTest report generated: " << config_.report_path << std::endl;
    }
    
    /**
     * @brief Count total tests
     */
    std::size_t count_total_tests() const {
        std::size_t count = 0;
        for (const auto& suite : suites_) {
            count += suite->get_tests().size();
        }
        return count;
    }
    
    /**
     * @brief Get separator line
     */
    std::string get_separator() const {
        return std::string(60, '=');
    }
    
    /**
     * @brief Convert status to string
     */
    std::string status_to_string(test_status status) const {
        switch (status) {
            case test_status::passed: return "PASS";
            case test_status::failed: return "FAIL";
            case test_status::skipped: return "SKIP";
            case test_status::timeout: return "TIMEOUT";
            default: return "UNKNOWN";
        }
    }
    
    /**
     * @brief Get current time string
     */
    std::string get_current_time() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    test_runner_config config_;
    std::shared_ptr<event_bus> event_bus_;
    std::shared_ptr<thread_pool> thread_pool_;
    
    std::vector<std::unique_ptr<test_suite>> suites_;
    std::vector<test_result> results_;
    std::vector<std::function<void(const test_result&)>> observers_;
    
    std::mutex results_mutex_;
    std::mutex print_mutex_;
    
    std::atomic<std::size_t> passed_count_{0};
    std::atomic<std::size_t> failed_count_{0};
    std::atomic<std::size_t> skipped_count_{0};
    std::atomic<std::size_t> timeout_count_{0};
};

} // namespace kcenon::testing