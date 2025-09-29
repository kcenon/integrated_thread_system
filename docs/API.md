# API Reference

## Table of Contents
- [Core Classes](#core-classes)
- [Configuration](#configuration)
- [Task Submission](#task-submission)
- [Enhanced Features](#enhanced-features)
- [Monitoring & Metrics](#monitoring--metrics)
- [Utility Types](#utility-types)

## Core Classes

### `unified_thread_system`

The main class that provides unified access to threading, logging, and monitoring functionality.

```cpp
namespace kcenon::integrated {
    class unified_thread_system;
}
```

#### Constructors

```cpp
explicit unified_thread_system(const config& cfg = config());
```
Creates a unified thread system with the specified configuration. If no configuration is provided, uses default settings with automatic detection.

#### Destructor

```cpp
~unified_thread_system();
```
Automatically handles graceful shutdown of all subsystems.

## Configuration

### `config`

Configuration structure for customizing system behavior.

```cpp
struct config {
    // Basic Configuration
    std::string name = "ThreadSystem";
    size_t thread_count = 0;  // 0 = auto-detect

    // Logging Configuration
    bool enable_file_logging = true;
    bool enable_console_logging = true;
    std::string log_directory = "./logs";
    log_level min_log_level = log_level::info;

    // Enhanced Features (optional)
    bool enable_circuit_breaker = false;
    size_t circuit_breaker_failure_threshold = 5;
    std::chrono::milliseconds circuit_breaker_reset_timeout{5000};
    size_t max_queue_size = 10000;
    bool enable_work_stealing = true;
    bool enable_dynamic_scaling = false;
    size_t min_threads = 1;
    size_t max_threads = 0;  // 0 = no limit

    // Builder pattern methods
    config& set_name(const std::string& n);
    config& set_worker_count(size_t c);
    config& set_logging(bool file, bool console);
};
```

## Task Submission

### Basic Task Submission

#### `submit`
```cpp
template<typename F, typename... Args>
auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
```
Submits a task for asynchronous execution.

**Parameters:**
- `f`: Function or callable object to execute
- `args`: Arguments to pass to the function

**Returns:** `std::future` containing the result

**Example:**
```cpp
auto future = system.submit([]() { return 42; });
int result = future.get();  // 42
```

### Batch Processing

#### `submit_batch`
```cpp
template<typename Iterator, typename F>
auto submit_batch(Iterator first, Iterator last, F&& func)
    -> std::vector<std::future<std::invoke_result_t<F, typename Iterator::value_type>>>;
```
Processes multiple items in parallel.

**Parameters:**
- `first`, `last`: Iterator range of items to process
- `func`: Function to apply to each item

**Returns:** Vector of futures containing results

**Example:**
```cpp
std::vector<int> data = {1, 2, 3, 4, 5};
auto futures = system.submit_batch(data.begin(), data.end(),
    [](int n) { return n * n; });
```

## Enhanced Features

### Priority-Based Submission

#### `submit_with_priority`
```cpp
template<typename F, typename... Args>
auto submit_with_priority(priority_level priority, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
Submits a task with specified priority.

#### `submit_critical`
```cpp
template<typename F, typename... Args>
auto submit_critical(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
Submits a high-priority task.

#### `submit_background`
```cpp
template<typename F, typename... Args>
auto submit_background(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
Submits a low-priority background task.

### Cancellation Support

#### `submit_cancellable`
```cpp
template<typename F, typename... Args>
auto submit_cancellable(cancellation_token& token, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
Submits a task that can be cancelled.

**Example:**
```cpp
cancellation_token token;
auto future = system.submit_cancellable(token, []() {
    // Long-running task
    return process_data();
});

// Cancel if needed
token.cancel();
```

### Scheduled Execution

#### `schedule`
```cpp
template<typename F, typename... Args>
auto schedule(std::chrono::milliseconds delay, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
Schedules a task to run after a delay.

#### `schedule_recurring`
```cpp
template<typename F>
size_t schedule_recurring(std::chrono::milliseconds interval, F&& f);
```
Schedules a task to run repeatedly at intervals.

**Returns:** Task ID for cancellation

#### `cancel_recurring`
```cpp
void cancel_recurring(size_t task_id);
```
Cancels a recurring task.

### Map-Reduce Pattern

#### `map_reduce`
```cpp
template<typename Iterator, typename MapFunc, typename ReduceFunc, typename T>
auto map_reduce(Iterator first, Iterator last,
                MapFunc&& map_func, ReduceFunc&& reduce_func, T initial)
    -> std::future<T>;
```
Performs parallel map-reduce operation.

**Example:**
```cpp
std::vector<int> numbers = {1, 2, 3, 4, 5};
auto sum_of_squares = system.map_reduce(
    numbers.begin(), numbers.end(),
    [](int n) { return n * n; },      // Map: square
    [](int a, int b) { return a + b; }, // Reduce: sum
    0                                    // Initial value
);
```

## Monitoring & Metrics

### Performance Metrics

#### `get_metrics`
```cpp
performance_metrics get_metrics() const;
```
Returns current performance metrics.

```cpp
struct performance_metrics {
    size_t tasks_submitted;
    size_t tasks_completed;
    size_t tasks_failed;
    size_t tasks_cancelled;

    std::chrono::nanoseconds average_latency;
    std::chrono::nanoseconds min_latency;
    std::chrono::nanoseconds max_latency;
    std::chrono::nanoseconds p95_latency;
    std::chrono::nanoseconds p99_latency;

    size_t active_workers;
    size_t queue_size;
    double queue_utilization_percent;
    double tasks_per_second;
};
```

### Health Status

#### `get_health`
```cpp
health_status get_health() const;
```
Returns system health information.

```cpp
struct health_status {
    health_level overall_health;
    double cpu_usage_percent;
    double memory_usage_percent;
    double queue_utilization_percent;
    bool circuit_breaker_open;
    size_t consecutive_failures;
    std::vector<std::string> issues;
};
```

### System Control

#### `wait_for_completion`
```cpp
void wait_for_completion();
```
Blocks until all queued tasks complete.

#### `wait_for_completion_timeout`
```cpp
bool wait_for_completion_timeout(std::chrono::milliseconds timeout);
```
Waits with timeout for task completion.

#### `shutdown`
```cpp
void shutdown();
```
Gracefully shuts down the system.

#### `shutdown_immediate`
```cpp
void shutdown_immediate();
```
Immediately shuts down, cancelling pending tasks.

#### `is_shutting_down`
```cpp
bool is_shutting_down() const;
```
Checks if system is shutting down.

### Worker Management

#### `worker_count`
```cpp
size_t worker_count() const;
```
Returns number of worker threads.

#### `set_worker_count`
```cpp
void set_worker_count(size_t count);
```
Dynamically adjusts worker thread count.

#### `set_work_stealing`
```cpp
void set_work_stealing(bool enabled);
```
Enables/disables work stealing between workers.

#### `queue_size`
```cpp
size_t queue_size() const;
```
Returns current queue size.

### Circuit Breaker

#### `reset_circuit_breaker`
```cpp
void reset_circuit_breaker();
```
Resets the circuit breaker after failures.

#### `is_circuit_open`
```cpp
bool is_circuit_open() const;
```
Checks if circuit breaker is open (blocking new tasks).

### Logging

#### `log`
```cpp
template<typename... Args>
void log(log_level level, const std::string& message, Args&&... args);
```
Logs a message with specified level.

**Log Levels:**
- `trace`: Detailed debugging information
- `debug`: Debug information
- `info`: Informational messages
- `warning`: Warning messages
- `error`: Error messages
- `critical`: Critical errors
- `fatal`: Fatal errors

### Event System

#### `subscribe_to_events`
```cpp
using event_callback = std::function<void(const std::string&, const std::any&)>;
size_t subscribe_to_events(const std::string& event_type, event_callback callback);
```
Subscribes to system events.

#### `unsubscribe_from_events`
```cpp
void unsubscribe_from_events(size_t subscription_id);
```
Unsubscribes from events.

### Plugin System

#### `load_plugin`
```cpp
void load_plugin(const std::string& plugin_path);
```
Loads a plugin dynamically.

#### `unload_plugin`
```cpp
void unload_plugin(const std::string& plugin_name);
```
Unloads a plugin.

#### `list_plugins`
```cpp
std::vector<std::string> list_plugins() const;
```
Lists loaded plugins.

### Export Functions

#### `export_metrics_json`
```cpp
std::string export_metrics_json() const;
```
Exports metrics in JSON format.

#### `export_metrics_prometheus`
```cpp
std::string export_metrics_prometheus() const;
```
Exports metrics in Prometheus format.

## Utility Types

### `priority_level`
```cpp
enum class priority_level {
    lowest = 0,
    low = 25,
    normal = 50,
    high = 75,
    highest = 100,
    critical = 127
};
```

### `health_level`
```cpp
enum class health_level {
    healthy,
    degraded,
    critical,
    failed
};
```

### `log_level`
```cpp
enum class log_level {
    trace,
    debug,
    info,
    warning,
    error,
    critical,
    fatal
};
```

### `cancellation_token`
```cpp
class cancellation_token {
public:
    cancellation_token();
    void cancel();
    bool is_cancelled() const;
};
```

## Error Handling

All methods provide strong exception safety guarantee. Tasks that throw exceptions will propagate the exception through the future's `get()` method.

```cpp
try {
    auto future = system.submit([]() {
        throw std::runtime_error("Task failed");
    });
    future.get();  // Throws the exception
} catch (const std::exception& e) {
    std::cerr << "Task error: " << e.what() << std::endl;
}
```

## Thread Safety

All public methods are thread-safe and can be called concurrently from multiple threads.

## Performance Considerations

- Task submission: O(log n) with priority queue
- Work stealing: Reduces idle time
- Batch processing: Minimizes submission overhead
- Circuit breaker: Prevents cascade failures
- Memory pooling: Reduces allocation overhead