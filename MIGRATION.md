# Migration Guide

This guide helps you migrate from individual systems (`thread_system`, `logger_system`, `monitoring_system`) to the unified `integrated_thread_system`.

## Table of Contents

- [Why Migrate?](#why-migrate)
- [Migration from thread_system](#migration-from-thread_system)
- [Migration from logger_system](#migration-from-logger_system)
- [Migration from monitoring_system](#migration-from-monitoring_system)
- [Migration Checklist](#migration-checklist)
- [Breaking Changes](#breaking-changes)
- [Compatibility Layer](#compatibility-layer)

---

## Why Migrate?

**Benefits of Integrated Thread System**:

| Feature | Individual Systems | Integrated System |
|---------|-------------------|-------------------|
| **Setup Complexity** | Configure 3 systems separately | Zero-configuration setup |
| **Include Count** | 3+ headers | 1 header |
| **Initialization** | Manual integration needed | Automatic integration |
| **Observability** | Manual logging/monitoring setup | Built-in by default |
| **Error Handling** | Inconsistent patterns | Unified `Result<T>` pattern |
| **Maintenance** | Track 3 repositories | Single repository |

**Performance**:
- **No overhead**: Integration uses adapters, not abstraction layers
- **Same throughput**: 2.48M+ jobs/sec (thread_system baseline)
- **Enhanced features**: Priority scheduling, circuit breakers, metrics aggregation

---

## Migration from thread_system

### Before (thread_system only)

```cpp
#include <kcenon/thread/thread_pool.h>
#include <kcenon/thread/job_queue.h>
#include <kcenon/thread/thread_worker.h>

using namespace kcenon::thread;

int main() {
    // Create job queue
    auto queue = std::make_shared<job_queue>();

    // Create thread pool
    auto pool = std::make_shared<thread_pool>();

    // Create workers manually
    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        auto worker = std::make_shared<thread_worker>();
        worker->set_job_queue(queue);
        pool->add_worker(worker);
    }

    // Submit task
    auto future = pool->submit([]() {
        return 42;
    });

    int result = future.get();

    // Manual cleanup
    pool->stop();
    pool->wait_for_completion();

    return 0;
}
```

### After (integrated_thread_system)

```cpp
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

int main() {
    // Zero-configuration setup
    unified_thread_system system;

    // Submit task (same API!)
    auto future = system.submit([]() {
        return 42;
    });

    int result = future.get();

    // Automatic cleanup on destruction
    return 0;
}
```

### Key Changes

1. **Include Path**:
   - Old: `#include <kcenon/thread/thread_pool.h>`
   - New: `#include <kcenon/integrated/unified_thread_system.h>`

2. **Namespace**:
   - Old: `kcenon::thread`
   - New: `kcenon::integrated`

3. **Initialization**:
   - Old: Manual creation of queue, pool, workers
   - New: Single constructor call

4. **Worker Management**:
   - Old: Manually create and add workers
   - New: Automatic based on configuration

### Configuration Migration

**Before (thread_system)**:
```cpp
auto pool = std::make_shared<thread_pool>();
pool->set_worker_count(8);
pool->enable_work_stealing(true);
```

**After (integrated_thread_system)**:
```cpp
config cfg;
cfg.thread_count = 8;
cfg.enable_work_stealing = true;

unified_thread_system system(cfg);
```

### Advanced Features

#### Cancellation Tokens

**Before (thread_system)**:
```cpp
#include <kcenon/thread/cancellation_token.h>

auto token = std::make_shared<kcenon::thread::cancellation_token>();

auto future = pool->submit_cancellable(token, []() {
    return 42;
});

token->cancel();
```

**After (integrated_thread_system)**:
```cpp
// Option 1: Use type-erased token (recommended)
auto token = system.create_cancellation_token();
auto future = system.submit_cancellable(token, []() {
    return 42;
});
system.cancel_token(token);

// Option 2: Use cancellation_token class directly
cancellation_token token;
auto future = system.submit_cancellable(token, []() {
    return 42;
});
token.cancel();
```

#### Priority Scheduling

**Before (thread_system with typed_thread_pool)**:
```cpp
#include <kcenon/thread/typed_thread_pool.h>

auto typed_pool = std::make_shared<typed_thread_pool>();
typed_pool->submit_with_priority(127, []() {  // Critical priority
    return 42;
});
```

**After (integrated_thread_system)**:
```cpp
// Option 1: Explicit priority
system.submit_with_priority(priority_level::critical, []() {
    return 42;
});

// Option 2: Convenience methods
system.submit_critical([]() { return 42; });
system.submit_background([]() { return 1; });
```

---

## Migration from logger_system

### Before (logger_system only)

```cpp
#include <kcenon/logger/logger.h>
#include <kcenon/logger/console_writer.h>
#include <kcenon/logger/file_writer.h>

using namespace kcenon::logger;

int main() {
    // Create logger
    auto logger = std::make_shared<logger>("MyApp");

    // Add console writer
    auto console = std::make_shared<console_writer>();
    logger->add_writer(console);

    // Add file writer
    auto file_writer = std::make_shared<file_writer>("./logs/app.log");
    logger->add_writer(file_writer);

    // Log messages
    logger->write(log_level::info, "Application started");

    // Manual cleanup
    logger->flush();

    return 0;
}
```

### After (integrated_thread_system)

```cpp
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

int main() {
    // Logging automatically configured
    config cfg;
    cfg.name = "MyApp";
    cfg.enable_console_logging = true;
    cfg.enable_file_logging = true;
    cfg.log_directory = "./logs";

    unified_thread_system system(cfg);

    // Log messages
    system.log(log_level::info, "Application started");

    // Automatic flush on destruction
    return 0;
}
```

### Key Changes

1. **Automatic Writer Setup**:
   - Old: Manually create and add writers
   - New: Configured via `config` struct

2. **Integrated Logging**:
   - Thread pool operations automatically logged
   - Task submissions and completions tracked
   - Performance metrics logged

3. **Log Levels**:
   - Same enum values: `trace`, `debug`, `info`, `warning`, `error`, `critical`, `fatal`
   - Available in `kcenon::integrated` namespace

### Configuration Migration

**Before (logger_system)**:
```cpp
auto logger = std::make_shared<logger>("MyApp");
logger->set_min_level(log_level::debug);
logger->set_async_mode(true, 8192);  // 8KB buffer
```

**After (integrated_thread_system)**:
```cpp
config cfg;
cfg.name = "MyApp";
cfg.min_log_level = log_level::debug;
cfg.logger.async_mode = true;
cfg.logger.buffer_size = 8192;

unified_thread_system system(cfg);
```

### Task-Integrated Logging

**New Feature**: Automatic logging of task lifecycle:

```cpp
unified_thread_system system;

// This task's submission and completion are automatically logged
auto future = system.submit([]() {
    return 42;
});

// Check logs:
// [info] Task submitted to thread pool
// [debug] Task started on worker 3
// [debug] Task completed in 125μs
```

To disable automatic logging:
```cpp
config cfg;
cfg.enable_console_logging = false;
cfg.enable_file_logging = false;  // Disable all logging

unified_thread_system system(cfg);
```

---

## Migration from monitoring_system

### Before (monitoring_system only)

```cpp
#include <kcenon/monitoring/performance_profiler.h>
#include <kcenon/monitoring/system_monitor.h>

using namespace kcenon::monitoring;

int main() {
    // Create profiler
    auto profiler = std::make_shared<performance_profiler>();

    // Create system monitor
    auto sys_monitor = std::make_shared<system_monitor>();

    // Manual metric recording
    profiler->start_operation("task_execution");
    // ... do work ...
    profiler->end_operation("task_execution");

    // Get metrics
    auto stats = profiler->get_statistics("task_execution");
    std::cout << "Mean: " << stats.mean_duration.count() << "ns" << std::endl;

    // Get system metrics
    auto cpu = sys_monitor->get_cpu_usage();
    auto mem = sys_monitor->get_memory_usage();

    return 0;
}
```

### After (integrated_thread_system)

```cpp
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

int main() {
    // Monitoring automatically configured
    unified_thread_system system;

    // Tasks are automatically profiled
    auto future = system.submit([]() {
        return 42;
    });
    future.get();

    // Get comprehensive metrics
    auto metrics = system.get_metrics();
    std::cout << "Average latency: " << metrics.average_latency.count() << "ns" << std::endl;
    std::cout << "P95 latency: " << metrics.p95_latency.count() << "ns" << std::endl;
    std::cout << "Throughput: " << metrics.tasks_per_second << " tasks/sec" << std::endl;

    // Get system health (includes CPU/memory)
    auto health = system.get_health();
    std::cout << "CPU usage: " << health.cpu_usage_percent << "%" << std::endl;
    std::cout << "Memory usage: " << health.memory_usage_percent << "%" << std::endl;

    return 0;
}
```

### Key Changes

1. **Automatic Profiling**:
   - Old: Manual `start_operation()` / `end_operation()` calls
   - New: Tasks automatically profiled

2. **Unified Metrics**:
   - Old: Separate profiler and system monitor
   - New: Single `get_metrics()` call

3. **Health Monitoring**:
   - Old: Manual threshold checking
   - New: Built-in health status with configurable thresholds

### Configuration Migration

**Before (monitoring_system)**:
```cpp
auto profiler = std::make_shared<performance_profiler>();
profiler->set_max_samples(10000);

auto sys_monitor = std::make_shared<system_monitor>();
sys_monitor->set_sampling_interval(std::chrono::milliseconds(100));
```

**After (integrated_thread_system)**:
```cpp
config cfg;
cfg.monitoring.max_samples_per_metric = 10000;
cfg.monitoring.sampling_interval_ms = 100;
cfg.monitoring.cpu_threshold_percent = 90.0;
cfg.monitoring.memory_threshold_percent = 90.0;

unified_thread_system system(cfg);
```

### Metrics Export

**Before (monitoring_system)**:
```cpp
auto profiler = std::make_shared<performance_profiler>();
// ... collect metrics ...

// Export to Prometheus (manual implementation)
std::string prometheus = export_to_prometheus(profiler);
```

**After (integrated_thread_system)**:
```cpp
unified_thread_system system;
// ... submit tasks ...

// Built-in export
std::string prometheus = system.export_metrics_prometheus();
std::string json = system.export_metrics_json();
```

---

## Migration Checklist

### Phase 1: Preparation

- [ ] Read this migration guide completely
- [ ] Review [DEPENDENCIES.md](DEPENDENCIES.md) for version requirements
- [ ] Check your code for compatibility issues (see [Breaking Changes](#breaking-changes))
- [ ] Set up a test environment

### Phase 2: Dependencies

- [ ] Clone `integrated_thread_system` repository
- [ ] Ensure all required dependencies are available:
  - [ ] `common_system`
  - [ ] `thread_system`
  - [ ] `logger_system`
  - [ ] `monitoring_system`
- [ ] Build dependencies
- [ ] Build `integrated_thread_system`

### Phase 3: Code Changes

- [ ] Update includes:
  - [ ] Replace `<kcenon/thread/...>` with `<kcenon/integrated/unified_thread_system.h>`
  - [ ] Replace `<kcenon/logger/...>` with `<kcenon/integrated/unified_thread_system.h>`
  - [ ] Replace `<kcenon/monitoring/...>` with `<kcenon/integrated/unified_thread_system.h>`
- [ ] Update namespaces:
  - [ ] Replace `kcenon::thread` with `kcenon::integrated`
  - [ ] Replace `kcenon::logger` with `kcenon::integrated`
  - [ ] Replace `kcenon::monitoring` with `kcenon::integrated`
- [ ] Update initialization code:
  - [ ] Replace manual pool/logger/monitor creation with `unified_thread_system`
  - [ ] Migrate configuration to `config` struct
- [ ] Update task submission:
  - [ ] `pool->submit()` → `system.submit()`
  - [ ] Update cancellation token usage if applicable
- [ ] Update logging:
  - [ ] Replace `logger->write()` with `system.log()`
  - [ ] Or rely on automatic logging
- [ ] Update monitoring:
  - [ ] Replace manual profiling with `system.get_metrics()`
  - [ ] Use `system.get_health()` for health checks

### Phase 4: Testing

- [ ] Compile and fix any build errors
- [ ] Run existing unit tests
- [ ] Add integration tests for the unified system
- [ ] Performance test to ensure no regression
- [ ] Test error handling and edge cases

### Phase 5: Deployment

- [ ] Update CI/CD pipelines
- [ ] Update documentation
- [ ] Monitor performance in staging
- [ ] Gradual rollout to production

---

## Breaking Changes

### API Changes

1. **Namespace Changes**:
   - `kcenon::thread::thread_pool` → `kcenon::integrated::unified_thread_system`
   - All types now in `kcenon::integrated` namespace

2. **Configuration**:
   - No more manual worker creation
   - Configuration via `config` struct instead of setter methods

3. **Error Handling**:
   - Adapters use `common::Result<T>` pattern
   - More explicit error reporting

4. **Initialization**:
   - Thread pool workers no longer auto-created by `thread_pool`
   - Adapters handle worker creation automatically

### Removed Features

1. **Direct Access to Internal Components**:
   - Cannot directly access `thread_pool`, `logger`, or `performance_profiler`
   - Use unified API instead

2. **Custom Writers** (temporary limitation):
   - Cannot add custom log writers yet
   - Built-in console and file writers only
   - Custom writers planned for future release

### Behavioral Changes

1. **Automatic Logging**:
   - Task submissions and completions logged by default
   - Disable with `cfg.enable_*_logging = false`

2. **Graceful Shutdown**:
   - Destructor waits for task completion by default
   - Use `shutdown_immediate()` for immediate shutdown

---

## Compatibility Layer

For gradual migration, you can use both systems side-by-side:

```cpp
#include <kcenon/thread/thread_pool.h>
#include <kcenon/integrated/unified_thread_system.h>

int main() {
    // Old code using thread_system
    auto old_pool = std::make_shared<kcenon::thread::thread_pool>();
    // ... configure old_pool ...

    // New code using integrated_thread_system
    kcenon::integrated::unified_thread_system new_system;

    // Submit to old pool
    auto old_future = old_pool->submit([]() { return 1; });

    // Submit to new system
    auto new_future = new_system.submit([]() { return 2; });

    // Both work concurrently
    std::cout << old_future.get() + new_future.get() << std::endl;

    return 0;
}
```

**Note**: This is not recommended for production due to resource duplication. Use for testing only.

---

## Example: Full Migration

### Before (All Three Systems)

```cpp
#include <kcenon/thread/thread_pool.h>
#include <kcenon/logger/logger.h>
#include <kcenon/monitoring/performance_profiler.h>

using namespace kcenon;

int main() {
    // Setup logger
    auto logger = std::make_shared<logger::logger>("MyApp");
    auto console = std::make_shared<logger::console_writer>();
    logger->add_writer(console);

    // Setup profiler
    auto profiler = std::make_shared<monitoring::performance_profiler>();

    // Setup thread pool
    auto queue = std::make_shared<thread::job_queue>();
    auto pool = std::make_shared<thread::thread_pool>();
    for (size_t i = 0; i < 8; ++i) {
        auto worker = std::make_shared<thread::thread_worker>();
        worker->set_job_queue(queue);
        pool->add_worker(worker);
    }

    // Log start
    logger->write(logger::log_level::info, "Application started");

    // Profile task execution
    profiler->start_operation("main_task");
    auto future = pool->submit([]() {
        return 42;
    });
    int result = future.get();
    profiler->end_operation("main_task");

    // Get stats
    auto stats = profiler->get_statistics("main_task");
    logger->write(logger::log_level::info,
        "Task completed in " + std::to_string(stats.mean_duration.count()) + "ns");

    // Cleanup
    pool->stop();
    pool->wait_for_completion();
    logger->flush();

    return 0;
}
```

### After (Integrated System)

```cpp
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

int main() {
    // Single configuration for everything
    config cfg;
    cfg.name = "MyApp";
    cfg.thread_count = 8;
    cfg.enable_console_logging = true;
    cfg.min_log_level = log_level::info;

    unified_thread_system system(cfg);

    // Logging automatic on construction
    // "Application started" is logged automatically

    // Task automatically profiled
    auto future = system.submit([]() {
        return 42;
    });
    int result = future.get();

    // Get metrics (includes profiling data)
    auto metrics = system.get_metrics();
    system.log(log_level::info,
        "Task completed in " + std::to_string(metrics.average_latency.count()) + "ns");

    // Automatic cleanup on destruction
    return 0;
}
```

**Lines of Code**:
- Before: ~35 lines
- After: ~20 lines
- **Reduction: 43%**

**Benefits**:
- Simpler initialization
- Automatic integration
- Less error-prone
- Built-in best practices

---

## Need Help?

- **Migration Issues**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
- **API Questions**: See [docs/API.md](docs/API.md)
- **Examples**: See [docs/EXAMPLES.md](docs/EXAMPLES.md)
- **GitHub Issues**: [Report migration problems](https://github.com/kcenon/integrated_thread_system/issues)

---

## See Also

- [README.md](README.md) - Project overview
- [DEPENDENCIES.md](DEPENDENCIES.md) - Dependency requirements
- [CHANGELOG.md](CHANGELOG.md) - Version history and breaking changes
- [docs/architecture/ARCHITECTURE.md](docs/architecture/ARCHITECTURE.md) - System architecture
