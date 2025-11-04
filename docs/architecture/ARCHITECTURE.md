# Integrated Thread System Architecture

## Overview

The Integrated Thread System combines three high-performance C++20 systems into a unified framework that provides enterprise-grade threading, logging, and monitoring capabilities through an **Adapter-Based Integration** pattern. This document details the architectural decisions, patterns, and implementation strategies used to achieve seamless integration while maintaining individual system performance.

**Architecture Pattern**: Adapter-Based Integration with PIMPL
**Design Philosophy**: Recreate the simplicity of the original thread_system with zero-configuration setup

## Architectural Principles

### 1. Simplicity First

The primary goal is to recreate the simplicity of the original `thread_system` before it was split into separate components:

```cpp
// Zero-configuration usage
unified_thread_system system;
auto future = system.submit([]() { return 42; });
auto result = future.get();
```

### 2. Adapter-Based Integration

Rather than creating complex facade layers, we use lightweight adapters that wrap each external system:

```
Application Layer
    ↓
unified_thread_system (Unified API)
    ↓
system_coordinator (Lifecycle Manager)
    ↓
┌───────────────┬──────────────────┬────────────────────┐
thread_adapter  logger_adapter     monitoring_adapter
    ↓               ↓                   ↓
thread_system   logger_system      monitoring_system
(external)      (external)         (external)
```

### 3. PIMPL (Pointer to Implementation)

All major classes use the PIMPL idiom for:
- **ABI Stability**: Binary interface remains stable across minor versions
- **Faster Compilation**: Changes to implementation don't require recompiling users
- **Hidden Dependencies**: External system headers not exposed in public API

```cpp
class unified_thread_system {
public:
    // Public API
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<...>;

private:
    class impl;  // Forward declaration
    std::unique_ptr<impl> pimpl_;  // Pointer to implementation
};
```

### 4. Result<T> Error Handling

All operations use the `common::Result<T>` pattern for explicit, type-safe error handling:

```cpp
// Adapter initialization returns Result<void>
auto result = adapter.initialize();
if (!result) {
    const auto& error = result.get_error();
    // Handle error with full context
}
```

### 5. Conditional Compilation

The system supports two build modes:

1. **External Systems Available** (`EXTERNAL_SYSTEMS_AVAILABLE` defined)
   - Uses actual thread_system, logger_system, monitoring_system

2. **Fallback Mode** (External systems not available)
   - Provides basic implementations for all adapters
   - Allows compilation and basic functionality without external dependencies

## System Components

### Core Layer

#### 1. Configuration (`src/core/configuration.h/cpp`)

Unified configuration structure combining all subsystems:

```cpp
namespace kcenon::integrated {
    // Master configuration
    struct unified_config {
        thread_config thread;
        logger_config logger;
        monitoring_config monitoring;
        circuit_breaker_config circuit_breaker;
    };

    // Thread pool configuration
    struct thread_config {
        std::string name = "ThreadSystem";
        size_t worker_count = 0;  // 0 = auto-detect
        bool enable_work_stealing = true;
        thread_pool_type pool_type = thread_pool_type::standard;
        size_t max_queue_size = 10000;
    };

    // Logger configuration
    struct logger_config {
        std::string name = "Logger";
        bool enable_file_logging = true;
        bool enable_console_logging = true;
        std::string log_directory = "./logs";
        log_level min_log_level = log_level::info;
        log_format format = log_format::timestamp;
        bool async_mode = true;
        size_t buffer_size = 8192;
    };

    // Monitoring configuration
    struct monitoring_config {
        std::string name = "Monitor";
        bool enable_profiling = true;
        bool enable_system_monitoring = true;
        size_t sampling_interval_ms = 100;
        double cpu_threshold_percent = 90.0;
        double memory_threshold_percent = 90.0;
        size_t max_samples_per_metric = 10000;
    };
}
```

**Key Features**:
- Builder pattern support for fluent configuration
- Smart defaults (zero-configuration works)
- Automatic resource detection (CPU cores, paths)

#### 2. System Coordinator (`src/core/system_coordinator.h/cpp`)

The central orchestrator managing lifecycle of all subsystems:

```cpp
namespace kcenon::integrated::core {
    class system_coordinator {
    public:
        explicit system_coordinator(const unified_config& config);
        ~system_coordinator();

        // Lifecycle management
        common::VoidResult initialize();
        common::VoidResult shutdown();

        // Adapter access
        std::shared_ptr<adapters::thread_adapter> get_thread_adapter();
        std::shared_ptr<adapters::logger_adapter> get_logger_adapter();
        std::shared_ptr<adapters::monitoring_adapter> get_monitoring_adapter();

    private:
        class impl;
        std::unique_ptr<impl> pimpl_;
    };
}
```

**Initialization Order** (dependency-aware):
1. Logger adapter (no dependencies)
2. Monitoring adapter (may use logger)
3. Thread adapter (may use logger and monitoring)

**Shutdown Order** (reverse of initialization):
1. Thread adapter (stop accepting tasks, wait for completion)
2. Monitoring adapter (flush metrics)
3. Logger adapter (flush logs last)

### Adapter Layer

#### 1. Thread Adapter (`src/adapters/thread_adapter.h/cpp`)

Wraps `thread_system`'s thread pool functionality:

**External Implementation** (`EXTERNAL_SYSTEMS_AVAILABLE`):
```cpp
class thread_adapter::impl {
private:
    std::shared_ptr<kcenon::thread::thread_pool> thread_pool_;
    std::shared_ptr<kcenon::thread::job_queue> job_queue_;
    std::vector<std::shared_ptr<kcenon::thread::thread_worker>> workers_;

    // Thread pool doesn't auto-create workers - we do it manually
    void create_workers(size_t count);
};
```

**Key Implementation Details**:
- **Manual Worker Creation**: Unlike many thread pools, `thread_system`'s thread_pool doesn't automatically create workers. The adapter creates workers and adds them to the pool.
- **Job Queue Integration**: Workers share a job queue via `set_job_queue()`
- **Cancellation Token Support**: Type-erased tokens (`std::shared_ptr<void>`) allow both external and fallback implementations
- **Priority Support**: Future enhancement for priority-based scheduling

**Fallback Implementation**:
- Uses `std::jthread` for workers
- Simple FIFO queue with mutex protection
- Basic cancellation via `std::atomic<bool>`

#### 2. Logger Adapter (`src/adapters/logger_adapter.h/cpp`)

Wraps `logger_system`'s async logging functionality:

**External Implementation**:
```cpp
class logger_adapter::impl {
private:
    std::shared_ptr<kcenon::logger::logger> logger_;
    std::shared_ptr<kcenon::logger::console_writer> console_writer_;
    std::shared_ptr<kcenon::logger::file_writer> file_writer_;

    // 8KB async buffer for high-performance logging
    static constexpr size_t buffer_size = 8192;
};
```

**Features**:
- **Async Logging**: 4.34M+ logs/sec throughput
- **Multiple Outputs**: Simultaneous console and file logging
- **Log Level Conversion**: Maps `integrated::log_level` to `logger::log_level`
- **Source Location**: Optional file/line/function tracking
- **Auto-flush**: Ensures logs are written on shutdown

**Fallback Implementation**:
- Direct `std::cout`/`std::cerr` output
- Thread-safe via mutex
- No async buffering

#### 3. Monitoring Adapter (`src/adapters/monitoring_adapter.h/cpp`)

Wraps `monitoring_system`'s performance profiling and system monitoring:

**External Implementation**:
```cpp
class monitoring_adapter::impl {
private:
    std::shared_ptr<kcenon::monitoring::performance_profiler> profiler_;
    std::shared_ptr<kcenon::monitoring::system_monitor> system_monitor_;

    // Health thresholds (configurable)
    double cpu_threshold_percent_ = 90.0;
    double memory_threshold_percent_ = 90.0;

    // Metrics storage
    size_t max_samples_per_metric_ = 10000;
};
```

**Features**:
- **Performance Profiling**: Statistical analysis (mean, p95, p99, call counts)
- **System Monitoring**: Real-time CPU and memory usage
- **Health Checks**: Threshold-based health evaluation
- **Metrics Aggregation**: Collects and exports metrics in Prometheus/JSON formats

**Implements**: `common::interfaces::IMonitor` interface for standardization

**Fallback Implementation**:
- Basic metric counters
- No system resource monitoring
- Simple health status

### Extension Layer

#### Metrics Aggregator (`src/extensions/metrics_aggregator.h/cpp`)

Aggregates metrics from all adapters into unified reports:

```cpp
namespace kcenon::integrated::extensions {
    class metrics_aggregator {
    public:
        struct unified_metrics {
            // Thread pool metrics
            size_t worker_count;
            size_t queue_size;
            size_t tasks_submitted;
            size_t tasks_completed;

            // Logger metrics
            size_t messages_written;
            size_t write_errors;

            // System metrics
            double cpu_usage_percent;
            double memory_usage_percent;

            // Custom metrics
            std::map<std::string, double> custom_metrics;
        };

        unified_metrics collect_metrics();
        std::string export_prometheus();
        std::string export_json();
    };
}
```

**Export Formats**:
- **Prometheus**: Standard metrics exposition format
- **JSON**: Structured format for custom integrations

### Primary API Layer

#### Unified Thread System (`src/unified_thread_system.h/cpp`)

The main user-facing API that ties everything together:

```cpp
namespace kcenon::integrated {
    class unified_thread_system {
    public:
        // Zero-configuration constructor
        explicit unified_thread_system(const config& cfg = config());

        // Standard Features
        template<typename F, typename... Args>
        auto submit(F&& f, Args&&... args) -> std::future<...>;

        template<typename Iterator, typename F>
        auto submit_batch(Iterator first, Iterator last, F&& func) -> std::vector<std::future<...>>;

        performance_metrics get_metrics() const;
        health_status get_health() const;
        void wait_for_completion();

        // Enhanced Features
        template<typename F, typename... Args>
        auto submit_with_priority(priority_level priority, F&& f, Args&&... args) -> std::future<...>;

        template<typename F, typename... Args>
        auto submit_critical(F&& f, Args&&... args) -> std::future<...>;

        template<typename F, typename... Args>
        auto submit_background(F&& f, Args&&... args) -> std::future<...>;

        std::shared_ptr<void> create_cancellation_token();
        void cancel_token(std::shared_ptr<void> token);

        template<typename F, typename... Args>
        auto submit_cancellable(std::shared_ptr<void> token, F&& f, Args&&... args) -> std::future<...>;

        template<typename F, typename... Args>
        auto schedule(std::chrono::milliseconds delay, F&& f, Args&&... args) -> std::future<...>;

        size_t schedule_recurring(std::chrono::milliseconds interval, F&& f);
        void cancel_recurring(size_t task_id);

        template<typename Iterator, typename MapFunc, typename ReduceFunc, typename T>
        auto map_reduce(Iterator first, Iterator last, MapFunc&& map_func,
                       ReduceFunc&& reduce_func, T initial) -> std::future<T>;

        // Export functions
        std::string export_metrics_json() const;
        std::string export_metrics_prometheus() const;

    private:
        class impl;
        std::unique_ptr<impl> pimpl_;
    };
}
```

**Implementation Note**: The `unified_thread_system::impl` class holds:
- `std::unique_ptr<system_coordinator>` - manages all adapters
- `std::unique_ptr<metrics_aggregator>` - aggregates metrics

## Data Flow Architecture

### 1. Task Submission Flow

```
User Application
    ↓ submit(task)
unified_thread_system (template instantiation)
    ↓ std::packaged_task wrapping
submit_internal(std::function<void()>)
    ↓ get system_coordinator
system_coordinator::get_thread_adapter()
    ↓ adapter delegation
thread_adapter::execute(task)
    ↓ conditional compilation
[External] thread_pool->execute(task)
    ↓ job queue
thread_worker execution
    ↓ result
std::future::get()
```

### 2. Logging Flow

```
Task Execution
    ↓ log_internal()
unified_thread_system::impl
    ↓ get logger_adapter
logger_adapter::log(level, message)
    ↓ level conversion
[External] logger::write(level, message)
    ↓ async buffer (8KB)
[console_writer, file_writer]
    ↓ output
[stdout, log file]
```

### 3. Monitoring Flow

```
Task Completion
    ↓ update_metrics()
unified_thread_system::impl
    ↓ get monitoring_adapter
monitoring_adapter::record_metric(name, value)
    ↓ statistical aggregation
[External] performance_profiler::record(name, duration)
    ↓ storage (max 10K samples per metric)
metrics_aggregator::collect_metrics()
    ↓ export
[Prometheus, JSON formats]
```

### 4. Initialization Flow

```
unified_thread_system constructor
    ↓ create system_coordinator
system_coordinator::initialize()
    ↓ dependency-aware initialization
1. logger_adapter::initialize()
    ↓ create logger with writers
2. monitoring_adapter::initialize()
    ↓ create profiler + system_monitor
3. thread_adapter::initialize()
    ↓ create thread_pool + workers
    ↓ set job_queue on workers
    ↓ start workers
Ready for task submission
```

### 5. Shutdown Flow

```
unified_thread_system destructor
    ↓ call system_coordinator::shutdown()
Reverse order shutdown:
1. thread_adapter::shutdown()
    ↓ stop accepting tasks
    ↓ wait_for_completion()
    ↓ stop workers
2. monitoring_adapter::shutdown()
    ↓ flush metrics
3. logger_adapter::shutdown()
    ↓ flush log buffer (last!)
Clean shutdown complete
```

## Error Handling Architecture

### Centralized Error Code Registry

To prevent conflicts, error codes are partitioned by component:

| Component | Error Code Range | Purpose |
|-----------|-----------------|---------|
| common_system | -1 to -99 | Foundation errors |
| thread_system | -100 to -199 | Thread pool errors |
| logger_system | -200 to -299 | Logging errors |
| monitoring_system | -300 to -399 | Monitoring errors |
| **integrated_thread_system** | **-400 to -499** | Integration layer errors |

**Integration Layer Error Codes**:
- -400 to -409: Component initialization failures
- -410 to -419: Configuration validation errors
- -420 to -429: Runtime coordination errors
- -430 to -449: Reserved for future use

### Error Propagation Pattern

```cpp
// Adapter initialization with Result<T>
auto thread_pool = thread_system::create_pool(config);
if (!thread_pool) {
    return propagate_error<unified_thread_system>(thread_pool.get_error());
}

auto logger = logger_system::create_logger(log_config);
if (!logger) {
    return propagate_error<unified_thread_system>(logger.get_error());
}

auto monitor = monitoring_system::create_monitor(monitor_config);
if (!monitor) {
    return propagate_error<unified_thread_system>(monitor.get_error());
}

// Successful integration
return ok(unified_thread_system{
    std::move(thread_pool.value()),
    std::move(logger.value()),
    std::move(monitor.value())
});
```

**Benefits**:
- Explicit error handling at integration boundaries
- Clear identification of which component failed
- Type-safe error propagation
- Comprehensive error context for debugging

### Exception Handling in Tasks

While adapters use `Result<T>`, user tasks can throw exceptions:

```cpp
auto future = system.submit([]() {
    throw std::runtime_error("Task failed");
});

try {
    future.get();  // Exception propagated through future
} catch (const std::exception& e) {
    // Handle task exception
}
```

**Implementation**: Exceptions are captured in `std::packaged_task` and propagated through `std::future`.

## Performance Optimizations

### 1. Zero-Allocation Fast Paths

- **Template Instantiation**: `submit()` uses templates to avoid type erasure overhead
- **Move Semantics**: Tasks moved (not copied) throughout submission pipeline
- **In-Place Construction**: `std::packaged_task` constructed directly

### 2. Lock-Free Operations

When `thread_system` is available:
- **MPMC Queues**: Multi-Producer Multi-Consumer lock-free job queues
- **Hazard Pointers**: Memory reclamation without locks
- **Work Stealing**: Lock-free deque per worker

### 3. Memory Pooling

External systems provide:
- **Job Object Pooling** (thread_system): Reduces allocations
- **Log Buffer Pooling** (logger_system): Reuses buffers
- **Metrics Storage Pooling** (monitoring_system): Ring buffers

### 4. Conditional Compilation

```cpp
#ifdef EXTERNAL_SYSTEMS_AVAILABLE
    // High-performance path with external systems
    thread_pool_->execute(std::move(task));
#else
    // Fallback path with basic implementation
    queue_.push(std::move(task));
#endif
```

**Benefits**:
- No runtime overhead for availability checks
- Compiler can optimize each path independently

## Thread Safety

### Concurrency Guarantees

| Component | Thread Safety | Mechanism |
|-----------|--------------|-----------|
| unified_thread_system | All methods thread-safe | PIMPL + internal synchronization |
| thread_adapter | Thread-safe task submission | Lock-free queues (external) |
| logger_adapter | Thread-safe logging | Async buffer + writer locks |
| monitoring_adapter | Thread-safe metrics | Atomic counters + internal locks |
| system_coordinator | Init/shutdown synchronized | Mutex-protected state machine |

### Synchronization Primitives

- **std::mutex**: Used sparingly for initialization/shutdown
- **std::atomic**: Used for counters and flags
- **Lock-free structures**: Preferred when available (thread_system)
- **Thread-local storage**: Avoided to reduce complexity

## Testing Architecture

### Test Organization

```
tests/
├── unit/                        # Unit tests for components
│   ├── test_basic_operations.cpp
│   ├── test_priority_scheduling.cpp
│   └── CMakeLists.txt
├── integration/                 # Cross-system integration tests
│   └── test_system_integration.cpp
├── benchmarks/                  # Performance benchmarks
│   └── bench_throughput.cpp
└── utils/                       # Test utilities
    ├── async_test_patterns.h
    ├── platform_test_config.h
    ├── test_wait_helper.h
    └── test_fixture_base.h
```

### Test Utilities

- **async_test_patterns.h**: Patterns for testing async operations
- **platform_test_config.h**: Platform-specific test configurations
- **test_wait_helper.h**: Timeout and waiting helpers
- **test_fixture_base.h**: Base fixture with common setup

### Test Coverage

**Target**: 85%+ combined coverage

**CI/CD Integration**:
- Multi-platform builds (Ubuntu, Windows, macOS)
- Multi-compiler testing (GCC, Clang, MSVC)
- Sanitizer validation (ASan, UBSan, TSan)
- Coverage tracking (lcov/gcovr)

## Build System Architecture

### CMake Configuration Strategy

**Dependency Search Order**:
1. `$HOME/Sources/<system_name>/include` (User home - developer priority)
2. `../system_name` (Sibling directory - CI priority)
3. `/usr/local/include` (System installation)
4. `${CMAKE_INSTALL_PREFIX}` (Custom prefix)
5. **FetchContent** from GitHub (Fallback with pinned versions)

**Benefits**:
- **Developer-Friendly**: Works with local checkouts
- **CI-Friendly**: Works with build matrices
- **User-Friendly**: Automatic download if not found

### Conditional Features

```cmake
option(BUILD_WITH_COMMON_SYSTEM "Build with common_system" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(ENABLE_WEB_DASHBOARD "Enable web dashboard" ON)
option(USE_SYSTEM_DEPENDENCIES "Use system-installed dependencies" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers" OFF)
option(INTEGRATED_ENABLE_COVERAGE "Enable coverage" OFF)
```

**Library Targets**:
- `integrated_thread_system`: Standard version (basic features)
- `integrated_thread_system_enhanced`: Enhanced version (all features)

### External System Detection

```cmake
# Try to find compiled thread_system library
find_library(THREAD_SYSTEM_LIB
    NAMES thread_system
    PATHS ${THREAD_SYSTEM_ROOT}/build/lib
          ${CMAKE_BINARY_DIR}/../thread_system/build/lib
)

# Fall back to header-only if library not found
if(NOT THREAD_SYSTEM_LIB)
    message(STATUS "Using header-only thread_system integration")
    target_compile_definitions(integrated_thread_system PRIVATE
        THREAD_SYSTEM_HEADER_ONLY)
endif()
```

**Flexibility**: Supports both linked libraries and header-only integration.

## Design Patterns Summary

| Pattern | Where Used | Purpose |
|---------|-----------|---------|
| **Adapter** | All adapter classes | Wrap external systems with unified interfaces |
| **PIMPL** | All major classes | Hide implementation, ABI stability |
| **Builder** | Configuration structs | Fluent configuration API |
| **Facade** | unified_thread_system | Simplify complex subsystem interactions |
| **Dependency Injection** | system_coordinator | Manage component lifecycle |
| **Result/Error** | All adapter operations | Explicit, type-safe error handling |
| **Type Erasure** | Cancellation tokens | Support multiple implementations |
| **Template Metaprogramming** | Task submission | Zero-overhead abstractions |

## Future Architecture Considerations

### Planned Enhancements

1. **Plugin System**
   - Dynamic loading of functionality
   - Plugin interface defined, implementation pending

2. **Distributed Tracing**
   - Cross-system trace correlation
   - OpenTelemetry integration

3. **Advanced Scheduling**
   - NUMA-aware work stealing
   - Dynamic thread pool sizing
   - Fairness guarantees

### Backward Compatibility Strategy

- **Public API**: Maintain stability across releases
- **Configuration**: Deprecate fields gradually with clear migration path
- **Error Codes**: Never reuse, only add new ranges
- **ABI**: PIMPL allows implementation changes without ABI breaks

### Migration Path

For users migrating from individual systems:
- `thread_system` users: Change include, minimal code changes
- `logger_system` users: Integrated logging automatic
- `monitoring_system` users: Metrics collection automatic

See [MIGRATION.md](../../MIGRATION.md) for detailed migration guide.

## Conclusion

The Integrated Thread System architecture prioritizes:
1. **Simplicity**: Zero-configuration for common cases
2. **Performance**: Near-zero overhead integration
3. **Flexibility**: Supports both external systems and fallback implementations
4. **Maintainability**: Clear separation of concerns, PIMPL for stability
5. **Observability**: Built-in logging and monitoring

By using adapters instead of complex facades, we achieve a clean, maintainable architecture that's easy to understand, test, and extend.

---

**Last Updated**: 2025-11-04
**Maintained By**: integrated_thread_system team
