/**
 * @file unified_thread_system.h
 * @brief Zero-configuration thread system with integrated logging and monitoring
 *
 * This header provides the main interface that recreates the simplicity of
 * the original thread_system before it was split into separate components.
 *
 * Usage:
 *   unified_thread_system system;
 *   auto future = system.submit([]{ return 42; });
 *   auto result = future.get();
 */

#pragma once

#include <future>
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

namespace kcenon::integrated {

/**
 * @brief Log levels matching the original thread_system
 */
enum class log_level {
    trace, debug, info, warning, error, critical
};

/**
 * @brief Performance metrics structure
 */
struct performance_metrics {
    size_t tasks_submitted{0};
    size_t tasks_completed{0};
    size_t tasks_failed{0};
    std::chrono::nanoseconds average_latency{0};
    size_t active_workers{0};
    size_t queue_size{0};
};

/**
 * @brief Health status levels
 */
enum class health_level {
    healthy, degraded, critical, failed
};

/**
 * @brief System health status
 */
struct health_status {
    health_level overall_health{health_level::healthy};
    double cpu_usage_percent{0.0};
    double memory_usage_percent{0.0};
    double queue_utilization_percent{0.0};
    std::vector<std::string> issues;
};

/**
 * @brief Main unified thread system class
 *
 * This class recreates the simplicity of the original thread_system by providing
 * a single point of entry that automatically manages logging, monitoring, and
 * thread pool functionality behind the scenes.
 */
class unified_thread_system {
public:
    /**
     * @brief Simple configuration options
     */
    struct config {
        std::string name = "ThreadSystem";
        size_t thread_count = 0; // 0 = auto-detect
        bool enable_file_logging = true;
        bool enable_console_logging = true;
        bool enable_monitoring = true;
        std::string log_directory = "./logs";
        log_level min_log_level = log_level::info;
    };

    /**
     * @brief Construct with automatic configuration
     *
     * Creates a fully functional thread system with logging and monitoring
     * enabled by default, matching the original thread_system behavior.
     */
    explicit unified_thread_system(const config& cfg);
    unified_thread_system(); // Default constructor

    /**
     * @brief Destructor automatically handles graceful shutdown
     */
    ~unified_thread_system();

    // Non-copyable but movable
    unified_thread_system(const unified_thread_system&) = delete;
    unified_thread_system& operator=(const unified_thread_system&) = delete;
    unified_thread_system(unified_thread_system&&) = default;
    unified_thread_system& operator=(unified_thread_system&&) = default;

    /**
     * @brief Submit a task (matches original thread_system simplicity)
     *
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return Future containing the result
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    /**
     * @brief Submit multiple tasks in batch
     *
     * @param first Iterator to first element
     * @param last Iterator past last element
     * @param func Function to apply to each element
     * @return Vector of futures containing results
     */
    template<typename Iterator, typename F>
    auto submit_batch(Iterator first, Iterator last, F&& func)
        -> std::vector<std::future<std::invoke_result_t<F, typename Iterator::value_type>>>;

    /**
     * @brief Get current performance metrics
     */
    performance_metrics get_metrics() const;

    /**
     * @brief Get current health status
     */
    health_status get_health() const;

    /**
     * @brief Manual logging (automatic logging already handles most cases)
     */
    template<typename... Args>
    void log(log_level level, const std::string& message, Args&&... args);

    /**
     * @brief Wait for all currently queued tasks to complete
     */
    void wait_for_completion();

    /**
     * @brief Get number of active worker threads
     */
    size_t worker_count() const;

    /**
     * @brief Get current queue size
     */
    size_t queue_size() const;

    /**
     * @brief Check if system is healthy
     */
    bool is_healthy() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl_;

    // Internal methods
    void submit_internal(std::function<void()> task);
    template<typename... Args>
    void log_internal(log_level level, const std::string& message, Args&&... args);
};

// Template implementations
template<typename F, typename... Args>
auto unified_thread_system::submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    auto result = task->get_future();

    // Implementation will be in the .cpp file
    submit_internal([task]() { (*task)(); });

    return result;
}

template<typename Iterator, typename F>
auto unified_thread_system::submit_batch(Iterator first, Iterator last, F&& func)
    -> std::vector<std::future<std::invoke_result_t<F, typename Iterator::value_type>>> {

    std::vector<std::future<std::invoke_result_t<F, typename Iterator::value_type>>> futures;

    for (auto it = first; it != last; ++it) {
        futures.push_back(submit(func, *it));
    }

    return futures;
}

// Template implementation moved to .cpp file for explicit instantiation

// Implementation is in the .cpp file

} // namespace kcenon::integrated

/**
 * @brief Convenience alias matching original naming
 */
using thread_system = kcenon::integrated::unified_thread_system;