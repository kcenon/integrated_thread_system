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
#include <any>
#include <atomic>
#include <optional>

namespace kcenon::integrated {

/**
 * @brief Log levels matching the original thread_system
 */
enum class log_level {
    trace, debug, info, warning, error, critical, fatal
};

/**
 * @brief Priority levels for task scheduling
 */
enum class priority_level {
    lowest = 0,
    low = 25,
    normal = 50,
    high = 75,
    highest = 100,
    critical = 127
};

/**
 * @brief Cancellation token for cancellable tasks
 */
class cancellation_token {
public:
    cancellation_token() : cancelled_(std::make_shared<std::atomic<bool>>(false)) {}

    void cancel() { cancelled_->store(true); }
    bool is_cancelled() const { return cancelled_->load(); }

private:
    std::shared_ptr<std::atomic<bool>> cancelled_;
};

/**
 * @brief Performance metrics structure
 */
struct performance_metrics {
    // Basic metrics
    size_t tasks_submitted{0};
    size_t tasks_completed{0};
    size_t tasks_failed{0};
    size_t tasks_cancelled{0};

    // Latency metrics
    std::chrono::nanoseconds average_latency{0};
    std::chrono::nanoseconds min_latency{0};
    std::chrono::nanoseconds max_latency{0};
    std::chrono::nanoseconds p95_latency{0};
    std::chrono::nanoseconds p99_latency{0};

    // Worker and queue metrics
    size_t active_workers{0};
    size_t queue_size{0};
    size_t max_queue_size{0};
    double queue_utilization_percent{0.0};

    // Throughput metrics
    double tasks_per_second{0.0};
    std::chrono::steady_clock::time_point measurement_start;
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

    // Circuit breaker status
    bool circuit_breaker_open{false};
    size_t consecutive_failures{0};

    std::vector<std::string> issues;
};

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

    // Enhanced features (optional)
    bool enable_circuit_breaker = false;
    size_t circuit_breaker_failure_threshold = 5;
    std::chrono::milliseconds circuit_breaker_reset_timeout{5000};
    size_t max_queue_size = 10000;
    bool enable_work_stealing = true;
    bool enable_dynamic_scaling = false;
    size_t min_threads = 1;
    size_t max_threads = 0; // 0 = no limit

    // Builder pattern for configuration
    config& set_name(const std::string& n) { name = n; return *this; }
    config& set_worker_count(size_t c) { thread_count = c; return *this; }
    config& set_logging(bool file, bool console) {
        enable_file_logging = file;
        enable_console_logging = console;
        return *this;
    }
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
    using config = kcenon::integrated::config;  // Use the outer config struct

    /**
     * @brief Construct with automatic configuration
     *
     * Creates a fully functional thread system with logging and monitoring
     * enabled by default, matching the original thread_system behavior.
     */
    explicit unified_thread_system(const config& cfg = config());
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
    void log(log_level level, const std::string& message, Args&&... args) {
        log_internal(level, message, std::forward<Args>(args)...);
    }

    /**
     * @brief Wait for all currently queued tasks to complete
     */
    void wait_for_completion();

    /**
     * @brief Wait with timeout
     */
    bool wait_for_completion_timeout(std::chrono::milliseconds timeout);

    /**
     * @brief Get number of active worker threads
     */
    size_t worker_count() const;

    /**
     * @brief Dynamically adjust worker thread count
     */
    void set_worker_count(size_t count);

    /**
     * @brief Enable/disable work stealing between workers
     */
    void set_work_stealing(bool enabled);

    /**
     * @brief Get current queue size
     */
    size_t queue_size() const;

    /**
     * @brief Check if system is healthy
     */
    bool is_healthy() const;

    /**
     * @brief Stop accepting new tasks and wait for completion
     */
    void shutdown();

    /**
     * @brief Emergency shutdown (cancel all pending tasks)
     */
    void shutdown_immediate();

    /**
     * @brief Check if the system is shutting down
     */
    bool is_shutting_down() const;

    /**
     * @brief Export metrics to various formats
     */
    std::string export_metrics_json() const;
    std::string export_metrics_prometheus() const;

    /**
     * @brief Enhanced task submission with priority
     */
    template<typename F, typename... Args>
    auto submit_with_priority(priority_level priority, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    template<typename F, typename... Args>
    auto submit_critical(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        return submit_with_priority(priority_level::critical, std::forward<F>(f), std::forward<Args>(args)...);
    }

    template<typename F, typename... Args>
    auto submit_background(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        return submit_with_priority(priority_level::low, std::forward<F>(f), std::forward<Args>(args)...);
    }

    /**
     * @brief Cancellable task submission
     */
    template<typename F, typename... Args>
    auto submit_cancellable(cancellation_token& token, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /**
     * @brief Scheduled task submission
     */
    template<typename F, typename... Args>
    auto schedule(std::chrono::milliseconds delay, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /**
     * @brief Recurring task submission
     */
    template<typename F>
    size_t schedule_recurring(std::chrono::milliseconds interval, F&& f);

    void cancel_recurring(size_t task_id);

    /**
     * @brief Map-reduce pattern
     */
    template<typename Iterator, typename MapFunc, typename ReduceFunc, typename T>
    auto map_reduce(Iterator first, Iterator last, MapFunc&& map_func,
                   ReduceFunc&& reduce_func, T initial) -> std::future<T>;

    /**
     * @brief Event subscription for monitoring
     */
    using event_callback = std::function<void(const std::string&, const std::any&)>;
    size_t subscribe_to_events(const std::string& event_type, event_callback callback);
    void unsubscribe_from_events(size_t subscription_id);

    /**
     * @brief Circuit breaker control
     */
    void reset_circuit_breaker();
    bool is_circuit_open() const;

    /**
     * @brief Plugin system support
     */
    void load_plugin(const std::string& plugin_path);
    void unload_plugin(const std::string& plugin_name);
    std::vector<std::string> list_plugins() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl_;

    // Internal methods
    void submit_internal(std::function<void()> task);
    void submit_priority_internal(int priority, std::function<void()> task);
    void schedule_internal(std::chrono::milliseconds delay, std::function<void()> task);
    size_t schedule_recurring_internal(std::chrono::milliseconds interval, std::function<void()> task);
    template<typename... Args>
    void log_internal(log_level level, const std::string& message, Args&&... args) {
        // Simple implementation for template
        std::string formatted = message;
        // In real implementation, would format with args
    }
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

template<typename F, typename... Args>
auto unified_thread_system::submit_with_priority(priority_level priority, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    auto result = task->get_future();

    // Submit with priority to internal implementation
    submit_priority_internal(static_cast<int>(priority), [task]() { (*task)(); });

    return result;
}

template<typename F, typename... Args>
auto unified_thread_system::submit_cancellable(cancellation_token& token, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        [token, func = std::bind(std::forward<F>(f), std::forward<Args>(args)...)]() -> return_type {
            if (token.is_cancelled()) {
                if constexpr (std::is_void_v<return_type>) {
                    return;
                } else {
                    return return_type{};
                }
            }
            return func();
        }
    );

    auto result = task->get_future();
    submit_internal([task]() { (*task)(); });

    return result;
}

template<typename F, typename... Args>
auto unified_thread_system::schedule(std::chrono::milliseconds delay, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    auto result = task->get_future();

    schedule_internal(delay, [task]() { (*task)(); });

    return result;
}

template<typename F>
size_t unified_thread_system::schedule_recurring(std::chrono::milliseconds interval, F&& f) {
    return schedule_recurring_internal(interval, std::forward<F>(f));
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

template<typename Iterator, typename MapFunc, typename ReduceFunc, typename T>
auto unified_thread_system::map_reduce(Iterator first, Iterator last, MapFunc&& map_func,
                               ReduceFunc&& reduce_func, T initial)
    -> std::future<T> {

    auto promise = std::make_shared<std::promise<T>>();
    auto future = promise->get_future();

    // Submit map phase
    auto map_futures = submit_batch(first, last, std::forward<MapFunc>(map_func));

    // Submit reduce phase
    submit([promise, futures = std::move(map_futures), reduce_func, initial]() mutable {
        T result = initial;
        for (auto& f : futures) {
            result = reduce_func(result, f.get());
        }
        promise->set_value(result);
    });

    return future;
}

// Template implementation moved to .cpp file for explicit instantiation

// Implementation is in the .cpp file

} // namespace kcenon::integrated

/**
 * @brief Convenience alias matching original naming
 */
using thread_system = kcenon::integrated::unified_thread_system;