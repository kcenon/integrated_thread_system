# Adapter Integration Guide

**Date:** 2025-11-12
**Version:** 2.0.0

This guide describes how to integrate the latest adapters from updated subsystems into `integrated_thread_system`.

---

## Overview

The `integrated_thread_system` uses adapter pattern to integrate with external systems. As of 2025-11-12, all subsystems have been updated with new adapters and features:

- **common_system v1.0.0**: Standalone event bus, Result<T> pattern, smart adapters
- **thread_system v1.0.0**: Scheduler interface, service registry, crash handler, common_system adapters
- **logger_system v1.0.0**: Backend architecture, formatters, security features, common_system adapters
- **monitoring_system v2.0.0**: Adaptive monitoring, health checks, reliability features, common_system adapters

---

## Adapter Updates Required

### 1. Thread Adapter Updates

#### Current Implementation
Location: `include/kcenon/integrated/adapters/thread_adapter.h`

#### Updates Needed (thread_system v1.0.0)

**New Adapters Available:**
- `kcenon::thread::adapters::common_system_executor_adapter` - Implements `common::executor_interface`
- `kcenon::thread::adapters::common_system_logger_adapter` - Implements `common::logger_interface`
- `kcenon::thread::adapters::common_system_monitoring_adapter` - Implements `common::monitoring_interface`

**New Features to Integrate:**
1. **Scheduler Interface** (`kcenon::thread::interfaces::scheduler_interface`)
   - Schedule tasks with delay
   - Recurring task scheduling
   - Task cancellation by ID

2. **Crash Handler** (`kcenon::thread::interfaces::crash_handler`)
   - Signal-safe crash recovery
   - Automatic thread recovery
   - Crash logging and diagnostics

3. **Service Registry** (`kcenon::thread::core::service_registry`)
   - Dependency injection container
   - Service lifecycle management
   - Interface registration and resolution

4. **Hazard Pointer** (`kcenon::thread::core::hazard_pointer`)
   - Lock-free memory reclamation
   - Safe concurrent pointer access
   - Experimental feature (requires `enable_hazard_pointer`)

5. **Bounded Job Queue** (`kcenon::thread::core::bounded_job_queue`)
   - Fixed-capacity queue
   - Backpressure support
   - Optional alternative to unbounded queue

**Configuration Integration:**
```cpp
// thread_config additions (already in configuration.h)
bool enable_scheduler = false;
bool enable_crash_handler = true;
bool enable_service_registry = true;
bool enable_hazard_pointer = false;
bool enable_bounded_queue = false;
std::size_t bounded_queue_capacity = 10000;
```

**Implementation Example:**
```cpp
// In thread_adapter.cpp
#include <kcenon/thread/adapters/common_system_executor_adapter.h>
#include <kcenon/thread/interfaces/scheduler_interface.h>
#include <kcenon/thread/core/service_registry.h>

class thread_adapter::impl {
public:
    impl(const thread_config& config) {
        // Use common_system_executor_adapter for standard interface
        executor_adapter_ = std::make_unique<
            kcenon::thread::adapters::common_system_executor_adapter>(
                config.pool_name, config.thread_count);

        // Optionally enable scheduler
        if (config.enable_scheduler) {
            scheduler_ = /* create scheduler */;
        }

        // Optionally enable service registry
        if (config.enable_service_registry) {
            registry_ = std::make_shared<kcenon::thread::core::service_registry>();
            // Register services
        }
    }

private:
    std::unique_ptr<kcenon::thread::adapters::common_system_executor_adapter> executor_adapter_;
    std::shared_ptr<kcenon::thread::interfaces::scheduler_interface> scheduler_;
    std::shared_ptr<kcenon::thread::core::service_registry> registry_;
};
```

---

### 2. Logger Adapter Updates

#### Current Implementation
Location: `include/kcenon/integrated/adapters/logger_adapter.h`

#### Updates Needed (logger_system v1.0.0)

**New Adapters Available:**
- `kcenon::logger::adapters::common_logger_adapter` - Implements `common::logger_interface`
- `kcenon::logger::adapters::common_system_adapter` - Generic common_system integration adapter
- `kcenon::logger::adapters::logger_adapter` - Generic logger adapter

**New Backend Architecture:**
- `kcenon::logger::backends::integration_backend` - For common_system integration
- `kcenon::logger::backends::standalone_backend` - For standalone mode
- `kcenon::logger::backends::thread_system_backend` - For thread_system integration
- `kcenon::logger::backends::monitoring_backend` - For monitoring_system integration

**New Formatters:**
- `kcenon::logger::formatters::base_formatter` - Base formatter interface
- `kcenon::logger::formatters::json_formatter` - JSON structured logging
- `kcenon::logger::formatters::timestamp_formatter` - Human-readable timestamp format

**New Security Features:**
- `kcenon::logger::security::audit_logger` - Audit trail logging
- `kcenon::logger::security::path_validator` - Secure path validation
- `kcenon::logger::security::secure_key_storage` - Encrypted key storage
- `kcenon::logger::security::signal_manager` - Signal handling for crash-safe logging

**Configuration Integration:**
```cpp
// logger_config additions (already in configuration.h)
log_format format = log_format::timestamp;  // or json, custom
bool pretty_print_json = false;
bool include_thread_id = true;
bool include_source_location = true;
bool enable_colors = true;
```

**Implementation Example:**
```cpp
// In logger_adapter.cpp
#include <kcenon/logger/adapters/common_logger_adapter.h>
#include <kcenon/logger/backends/integration_backend.h>
#include <kcenon/logger/formatters/json_formatter.h>
#include <kcenon/logger/formatters/timestamp_formatter.h>

class logger_adapter::impl {
public:
    impl(const logger_config& config) {
        // Create backend based on integration mode
        auto backend = std::make_shared<kcenon::logger::backends::integration_backend>();

        // Create formatter based on config
        std::shared_ptr<kcenon::logger::formatters::base_formatter> formatter;
        if (config.format == log_format::json) {
            formatter = std::make_shared<kcenon::logger::formatters::json_formatter>(
                config.pretty_print_json);
        } else {
            formatter = std::make_shared<kcenon::logger::formatters::timestamp_formatter>(
                config.enable_colors);
        }

        // Use common_logger_adapter
        logger_adapter_ = std::make_unique<
            kcenon::logger::adapters::common_logger_adapter>(
                backend, formatter, config.buffer_size);
    }

private:
    std::unique_ptr<kcenon::logger::adapters::common_logger_adapter> logger_adapter_;
};
```

---

### 3. Monitoring Adapter Updates

#### Current Implementation
Location: `include/kcenon/integrated/adapters/monitoring_adapter.h`

#### Updates Needed (monitoring_system v2.0.0)

**New Adapters Available:**
- `kcenon::monitoring::adapters::common_monitor_adapter` - Implements `common::monitoring_interface`
- `kcenon::monitoring::adapters::common_system_adapter` - Generic common_system integration
- `kcenon::monitoring::adapters::performance_monitor_adapter` - Performance monitoring adapter
- `kcenon::monitoring::adapters::thread_system_adapter` - Thread system integration adapter
- `kcenon::monitoring::adapters::logger_system_adapter` - Logger system integration adapter

**New Adaptive Monitoring:**
- `kcenon::monitoring::adaptive::adaptive_monitor` - Auto-adjust sampling based on load

**New Collectors:**
- `kcenon::monitoring::collectors::thread_system_collector` - Thread pool metrics
- `kcenon::monitoring::collectors::logger_system_collector` - Logger metrics
- `kcenon::monitoring::collectors::system_resource_collector` - CPU/memory metrics
- `kcenon::monitoring::collectors::plugin_metric_collector` - Plugin metrics

**New Health Monitoring:**
- `kcenon::monitoring::health::health_monitor` - System health monitoring

**New Reliability Features:**
- `kcenon::monitoring::reliability::circuit_breaker` - Circuit breaker pattern
- `kcenon::monitoring::reliability::error_boundary` - Error isolation
- `kcenon::monitoring::reliability::fault_tolerance_manager` - Fault tolerance
- `kcenon::monitoring::reliability::retry_policy` - Automatic retry

**Configuration Integration:**
```cpp
// monitoring_config additions (already in configuration.h)
bool enable_adaptive_monitoring = true;
double adaptive_low_threshold = 0.3;
double adaptive_high_threshold = 0.7;
std::chrono::milliseconds adaptive_min_interval{100};
std::chrono::milliseconds adaptive_max_interval{5000};

bool enable_health_monitoring = true;
std::chrono::milliseconds health_check_interval{5000};
bool enable_circuit_breaker_monitoring = true;

bool enable_thread_system_collector = true;
bool enable_logger_system_collector = true;
bool enable_system_resource_collector = true;
bool enable_plugin_metric_collector = false;

bool enable_error_boundary = true;
bool enable_fault_tolerance = true;
bool enable_retry_policy = false;
std::size_t max_retry_attempts = 3;
std::chrono::milliseconds retry_backoff_base{100};
```

**Implementation Example:**
```cpp
// In monitoring_adapter.cpp
#include <kcenon/monitoring/adapters/common_monitor_adapter.h>
#include <kcenon/monitoring/adaptive/adaptive_monitor.h>
#include <kcenon/monitoring/health/health_monitor.h>
#include <kcenon/monitoring/collectors/thread_system_collector.h>
#include <kcenon/monitoring/collectors/system_resource_collector.h>

class monitoring_adapter::impl {
public:
    impl(const monitoring_config& config) {
        // Use common_monitor_adapter
        monitor_adapter_ = std::make_unique<
            kcenon::monitoring::adapters::common_monitor_adapter>();

        // Enable adaptive monitoring if configured
        if (config.enable_adaptive_monitoring) {
            adaptive_monitor_ = std::make_shared<
                kcenon::monitoring::adaptive::adaptive_monitor>(
                    config.adaptive_low_threshold,
                    config.adaptive_high_threshold,
                    config.adaptive_min_interval,
                    config.adaptive_max_interval);
        }

        // Enable health monitoring if configured
        if (config.enable_health_monitoring) {
            health_monitor_ = std::make_shared<
                kcenon::monitoring::health::health_monitor>(
                    config.health_check_interval);
        }

        // Register collectors
        if (config.enable_thread_system_collector) {
            thread_collector_ = std::make_shared<
                kcenon::monitoring::collectors::thread_system_collector>();
            // Register with monitor
        }

        if (config.enable_system_resource_collector) {
            resource_collector_ = std::make_shared<
                kcenon::monitoring::collectors::system_resource_collector>();
            // Register with monitor
        }
    }

private:
    std::unique_ptr<kcenon::monitoring::adapters::common_monitor_adapter> monitor_adapter_;
    std::shared_ptr<kcenon::monitoring::adaptive::adaptive_monitor> adaptive_monitor_;
    std::shared_ptr<kcenon::monitoring::health::health_monitor> health_monitor_;
    std::shared_ptr<kcenon::monitoring::collectors::thread_system_collector> thread_collector_;
    std::shared_ptr<kcenon::monitoring::collectors::system_resource_collector> resource_collector_;
};
```

---

## Migration Checklist

### Phase 1: Configuration (✅ Completed)
- [x] Add thread system scheduler options to `thread_config`
- [x] Add monitoring adaptive options to `monitoring_config`
- [x] Add monitoring health options to `monitoring_config`
- [x] Add monitoring collector options to `monitoring_config`
- [x] Add monitoring reliability options to `monitoring_config`
- [x] Update CHANGELOG.md with configuration changes
- [x] Update README.md with version dependencies

### Phase 2: Adapter Headers (📝 In Progress)
- [ ] Update `thread_adapter.h` with scheduler, service registry, crash handler support
- [ ] Update `logger_adapter.h` with backend architecture and formatter support
- [ ] Update `monitoring_adapter.h` with adaptive monitoring and health monitoring support
- [ ] Add interface methods for new features
- [ ] Document new adapter methods in headers

### Phase 3: Adapter Implementation (⏳ Pending)
- [ ] Implement thread adapter with common_system_executor_adapter
- [ ] Implement logger adapter with common_logger_adapter and backends
- [ ] Implement monitoring adapter with common_monitor_adapter
- [ ] Add scheduler support to thread adapter
- [ ] Add service registry support to thread adapter
- [ ] Add crash handler support to thread adapter
- [ ] Add adaptive monitoring to monitoring adapter
- [ ] Add health monitoring to monitoring adapter
- [ ] Add collectors to monitoring adapter
- [ ] Add reliability features to monitoring adapter

### Phase 4: System Coordinator Integration (⏳ Pending)
- [ ] Update `system_coordinator.h` to expose new adapter features
- [ ] Update `system_coordinator.cpp` initialization order
- [ ] Add scheduler lifecycle management
- [ ] Add health monitoring lifecycle management
- [ ] Add adaptive monitoring lifecycle management

### Phase 5: Unified Thread System Updates (⏳ Pending)
- [ ] Add scheduler methods to `unified_thread_system`
- [ ] Add health monitoring methods to `unified_thread_system`
- [ ] Update performance metrics with new monitoring data
- [ ] Update health status with adaptive monitoring data
- [ ] Add examples demonstrating new features

### Phase 6: Testing & Validation (⏳ Pending)
- [ ] Write unit tests for new adapter features
- [ ] Write integration tests for subsystem interactions
- [ ] Benchmark performance with new features enabled/disabled
- [ ] Update CI/CD workflows
- [ ] Update documentation

---

## Best Practices

1. **Backward Compatibility**: All new features should be opt-in via configuration flags
2. **Graceful Degradation**: System should work even if some features are disabled
3. **Error Handling**: Use `common::Result<T>` for all operations that can fail
4. **Resource Management**: Use RAII and smart pointers for all resources
5. **Thread Safety**: Ensure all new code is thread-safe
6. **Documentation**: Update headers, examples, and user guides

---

## Related Documentation

- [CHANGELOG.md](../CHANGELOG.md) - Full changelog of updates
- [README.md](../README.md) - Project overview and version dependencies
- [Configuration Header](../include/kcenon/integrated/core/configuration.h) - Updated configuration options

---

## Support

For questions or issues with adapter integration:
- Open an issue: https://github.com/kcenon/integrated_thread_system/issues
- Review subsystem documentation:
  - [thread_system](https://github.com/kcenon/thread_system)
  - [logger_system](https://github.com/kcenon/logger_system)
  - [monitoring_system](https://github.com/kcenon/monitoring_system)
  - [common_system](https://github.com/kcenon/common_system)
