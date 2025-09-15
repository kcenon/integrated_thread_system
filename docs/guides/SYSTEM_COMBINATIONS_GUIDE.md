# System Combinations Guide

This guide explains how to selectively enable and configure different combinations of the three core systems (Thread, Logger, Monitoring) in the Integrated Thread System.

## Table of Contents

1. [Overview](#overview)
2. [Configuration Matrix](#configuration-matrix)
3. [Single System Configurations](#single-system-configurations)
4. [Dual System Configurations](#dual-system-configurations)
5. [Full System Configuration](#full-system-configuration)
6. [Performance Comparison](#performance-comparison)
7. [Selection Guidelines](#selection-guidelines)
8. [Migration Strategies](#migration-strategies)

## Overview

The Integrated Thread System allows you to enable only the systems you need, reducing overhead and complexity for applications that don't require all features.

### Benefits of Selective Configuration

- **Reduced Memory Footprint**: Only load what you need
- **Lower CPU Overhead**: Disabled systems consume zero resources
- **Simplified Debugging**: Fewer moving parts to troubleshoot
- **Better Performance**: Optimized for your specific use case
- **Gradual Adoption**: Start simple, add features as needed

## Configuration Matrix

| Configuration | Thread | Logger | Monitor | Use Case | Memory | CPU |
|--------------|--------|--------|---------|----------|--------|-----|
| Thread-Only | ✓ | ✗ | ✗ | Pure computation | Minimal | Lowest |
| Logger-Only | ✗ | ✓ | ✗ | Sequential logging | Minimal | Lowest |
| Monitor-Only | ✗ | ✗ | ✓ | Metrics collection | Low | Low |
| Thread+Logger | ✓ | ✓ | ✗ | Async processing with logs | Low | Medium |
| Thread+Monitor | ✓ | ✗ | ✓ | Performance-critical apps | Medium | Medium |
| Logger+Monitor | ✗ | ✓ | ✓ | Observability tools | Low | Low |
| All Systems | ✓ | ✓ | ✓ | Production applications | High | Highest |

## Single System Configurations

### 1. Thread-Only Configuration

**When to Use:**
- High-performance computing
- Embedded systems with limited resources
- Pure computational workloads
- Algorithm testing and benchmarking

**Configuration:**
```cpp
config cfg;
cfg.enable_thread_system(true)
   .enable_logger_system(false)
   .enable_monitoring_system(false)
   .set_worker_count(std::thread::hardware_concurrency())
   .set_queue_capacity(1000)
   .enable_adaptive_optimization(true);

unified_thread_system system(cfg);
```

**Key Features Available:**
- Parallel task execution
- Priority-based scheduling
- Adaptive queue optimization
- Work stealing
- Futures and promises

**Example Use Cases:**
```cpp
// Parallel computation
auto future = system.submit([]() {
    return compute_expensive_operation();
});

// Map-reduce pattern
std::vector<std::future<int>> futures;
for (const auto& data : dataset) {
    futures.push_back(system.submit([data]() {
        return process(data);
    }));
}
```

**Performance Characteristics:**
- **Memory**: ~2MB base overhead
- **Throughput**: 1M+ tasks/second
- **Latency**: < 100μs task submission
- **CPU**: Minimal overhead, all cores for computation

### 2. Logger-Only Configuration

**When to Use:**
- Command-line tools
- Configuration utilities
- Sequential applications
- Debugging tools

**Configuration:**
```cpp
config cfg;
cfg.enable_thread_system(false)
   .enable_logger_system(true)
   .enable_monitoring_system(false)
   .set_log_level(log_level::info)
   .set_log_file("application.log")
   .set_log_rotation_size(10 * 1024 * 1024);

unified_thread_system system(cfg);
```

**Key Features Available:**
- Synchronous logging
- Log levels (debug, info, warning, error, critical)
- Structured logging with metadata
- Log rotation
- Context-based logging

**Example Use Cases:**
```cpp
// Simple logging
system.log_info("Application started");

// Structured logging
system.log_error("Operation failed",
    {{"error_code", 500},
     {"user_id", "user_123"},
     {"timestamp", std::chrono::system_clock::now()}});

// Context logging
system.set_log_context({{"session_id", "abc123"}});
system.log_info("User action");  // Includes session_id
```

**Performance Characteristics:**
- **Memory**: < 1MB overhead
- **Throughput**: 100K+ logs/second
- **Latency**: Synchronous, immediate
- **CPU**: Minimal, I/O bound

### 3. Monitor-Only Configuration

**When to Use:**
- Health check endpoints
- Metrics exporters
- System monitors
- Dashboard backends

**Configuration:**
```cpp
config cfg;
cfg.enable_thread_system(false)
   .enable_logger_system(false)
   .enable_monitoring_system(true)
   .set_metrics_interval(std::chrono::seconds(1))
   .enable_system_metrics(true);

unified_thread_system system(cfg);
```

**Key Features Available:**
- System metrics collection
- Custom metrics registration
- Health checks
- Alert thresholds
- Multiple export formats

**Example Use Cases:**
```cpp
// Register custom metric
system.register_metric("request_count", metric_type::counter);
system.increment_counter("request_count");

// Health check
system.register_health_check("database", []() {
    return check_database_connection();
});

// Export metrics
auto prometheus_data = system.export_metrics(export_format::prometheus);
```

**Performance Characteristics:**
- **Memory**: ~5MB for metrics storage
- **Throughput**: 1M+ metric updates/second
- **Latency**: < 1μs metric update
- **CPU**: < 1% for collection

## Dual System Configurations

### 4. Thread + Logger Configuration

**When to Use:**
- Application servers
- Background job processors
- Data pipelines
- Microservices

**Configuration:**
```cpp
config cfg;
cfg.enable_thread_system(true)
   .enable_logger_system(true)
   .enable_monitoring_system(false)
   .set_worker_count(4)
   .enable_async_logging(true)
   .set_log_level(log_level::info);

unified_thread_system system(cfg);
```

**Benefits:**
- Parallel execution with detailed logging
- Async logging doesn't block workers
- Complete execution audit trail
- Error tracking across threads

**Example Pattern:**
```cpp
// Tracked task execution
auto future = system.submit([this]() {
    system.log_info("Task started");
    try {
        auto result = perform_work();
        system.log_info("Task completed successfully");
        return result;
    } catch (const std::exception& e) {
        system.log_error("Task failed: {}", e.what());
        throw;
    }
});
```

### 5. Thread + Monitor Configuration

**When to Use:**
- Performance-critical applications
- Real-time systems
- Load balancers
- Resource-intensive services

**Configuration:**
```cpp
config cfg;
cfg.enable_thread_system(true)
   .enable_logger_system(false)
   .enable_monitoring_system(true)
   .set_worker_count(8)
   .enable_performance_monitoring(true)
   .set_metrics_interval(std::chrono::milliseconds(100));

unified_thread_system system(cfg);
```

**Benefits:**
- Real-time performance metrics
- Queue depth monitoring
- Worker utilization tracking
- No logging overhead

**Example Pattern:**
```cpp
// Performance-monitored execution
system.register_metric("task_latency_ms", metric_type::gauge);

auto future = system.submit([this]() {
    auto start = std::chrono::steady_clock::now();
    auto result = perform_work();
    auto duration = std::chrono::steady_clock::now() - start;

    system.set_gauge("task_latency_ms",
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());

    return result;
});
```

### 6. Logger + Monitor Configuration

**When to Use:**
- Observability platforms
- Monitoring dashboards
- Audit systems
- Compliance tools

**Configuration:**
```cpp
config cfg;
cfg.enable_thread_system(false)
   .enable_logger_system(true)
   .enable_monitoring_system(true)
   .set_log_level(log_level::info)
   .enable_metrics_from_logs(true);

unified_thread_system system(cfg);
```

**Benefits:**
- Correlated logs and metrics
- Single source of truth
- Simplified observability
- No threading complexity

## Full System Configuration

### 7. All Systems Enabled

**When to Use:**
- Production applications
- Enterprise services
- Cloud-native applications
- Mission-critical systems

**Configuration:**
```cpp
config cfg;
cfg.enable_thread_system(true)
   .enable_logger_system(true)
   .enable_monitoring_system(true)
   .set_worker_count(std::thread::hardware_concurrency())
   .enable_async_logging(true)
   .enable_adaptive_optimization(true)
   .enable_performance_monitoring(true)
   .set_alert_threshold("error_rate", 0.05, alert_severity::critical);

unified_thread_system system(cfg);
```

**Complete Feature Set:**
- Parallel execution with priorities
- Comprehensive logging
- Real-time metrics
- Health checks
- Alerting
- Full observability

## Performance Comparison

### Memory Usage

| Configuration | Base Memory | Per 1K Tasks | Per 1M Logs | Per 1K Metrics |
|--------------|-------------|--------------|-------------|----------------|
| Thread-Only | 2MB | 100KB | - | - |
| Logger-Only | 1MB | - | 10MB | - |
| Monitor-Only | 5MB | - | - | 1MB |
| Thread+Logger | 3MB | 100KB | 10MB | - |
| Thread+Monitor | 7MB | 100KB | - | 1MB |
| Logger+Monitor | 6MB | - | 10MB | 1MB |
| All Systems | 8MB | 100KB | 10MB | 1MB |

### CPU Overhead

| Configuration | Idle CPU | Active CPU | Peak CPU |
|--------------|----------|------------|----------|
| Thread-Only | < 0.1% | 1-2% | 100% |
| Logger-Only | < 0.1% | 0.5% | 5% |
| Monitor-Only | 0.5% | 1% | 2% |
| Thread+Logger | < 0.1% | 2-3% | 100% |
| Thread+Monitor | 0.5% | 2-3% | 100% |
| Logger+Monitor | 0.5% | 1.5% | 7% |
| All Systems | 0.5% | 3-5% | 100% |

### Throughput Comparison

| Configuration | Tasks/sec | Logs/sec | Metrics/sec |
|--------------|-----------|----------|-------------|
| Thread-Only | 1M+ | - | - |
| Logger-Only | - | 100K | - |
| Monitor-Only | - | - | 1M+ |
| Thread+Logger | 900K | 90K | - |
| Thread+Monitor | 950K | - | 900K |
| Logger+Monitor | - | 95K | 950K |
| All Systems | 850K | 85K | 850K |

## Selection Guidelines

### Decision Tree

```
Start: What is your primary need?
│
├─ Parallel Processing?
│  ├─ Yes → Need Logging?
│  │  ├─ Yes → Need Metrics?
│  │  │  ├─ Yes → Use All Systems
│  │  │  └─ No → Use Thread+Logger
│  │  └─ No → Need Metrics?
│  │     ├─ Yes → Use Thread+Monitor
│  │     └─ No → Use Thread-Only
│  └─ No → Need Logging?
│     ├─ Yes → Need Metrics?
│     │  ├─ Yes → Use Logger+Monitor
│     │  └─ No → Use Logger-Only
│     └─ No → Need Metrics?
│        ├─ Yes → Use Monitor-Only
│        └─ No → Consider if you need this library
```

### Recommendations by Application Type

| Application Type | Recommended Configuration | Rationale |
|-----------------|--------------------------|-----------|
| HPC/Scientific Computing | Thread-Only | Maximum compute performance |
| Web API Server | Thread+Logger | Request handling with audit trail |
| Batch Processor | Thread+Logger | Job tracking and error logging |
| Monitoring Agent | Monitor-Only | Lightweight metrics collection |
| Debug Tool | Logger-Only | Simple sequential logging |
| Microservice | All Systems | Full observability |
| Embedded System | Thread-Only | Minimal resource usage |
| Lambda Function | Thread+Monitor | Fast execution with metrics |

## Migration Strategies

### Starting Simple

**Phase 1: Thread-Only**
```cpp
// Start with basic threading
config cfg;
cfg.enable_thread_system(true);
unified_thread_system system(cfg);
```

**Phase 2: Add Logging**
```cpp
// Add logging when needed
cfg.enable_logger_system(true)
   .set_log_file("app.log");
system.reconfigure(cfg);
```

**Phase 3: Add Monitoring**
```cpp
// Add monitoring for production
cfg.enable_monitoring_system(true)
   .enable_performance_monitoring(true);
system.reconfigure(cfg);
```

### Gradual Feature Adoption

1. **Development**: Start with Thread+Logger for debugging
2. **Testing**: Add Monitor for performance analysis
3. **Staging**: Enable all systems for full observability
4. **Production**: Fine-tune configuration based on metrics

### Configuration Profiles

```cpp
// Development profile
config dev_config()
{
    return config{}
        .enable_thread_system(true)
        .enable_logger_system(true)
        .enable_monitoring_system(false)
        .set_log_level(log_level::debug);
}

// Production profile
config prod_config()
{
    return config{}
        .enable_thread_system(true)
        .enable_logger_system(true)
        .enable_monitoring_system(true)
        .set_log_level(log_level::warning)
        .enable_async_logging(true)
        .enable_adaptive_optimization(true);
}

// Use based on environment
auto cfg = (env == "production") ? prod_config() : dev_config();
unified_thread_system system(cfg);
```

## Best Practices

### 1. Start Minimal
- Begin with only the systems you need
- Add features as requirements grow
- Measure impact of each addition

### 2. Profile Before Optimizing
- Use Monitor-Only to baseline performance
- Add systems and measure impact
- Optimize based on actual metrics

### 3. Environment-Specific Configurations
- Development: All systems for debugging
- Testing: Focus on functionality
- Production: Balance features vs overhead

### 4. Regular Review
- Monitor resource usage
- Disable unused features
- Adjust configuration based on metrics

### 5. Documentation
- Document why each system is enabled
- Record configuration decisions
- Maintain configuration changelog

## Conclusion

The Integrated Thread System's modular architecture allows you to:
- **Start simple** with just what you need
- **Scale up** by enabling additional systems
- **Optimize** for your specific use case
- **Maintain flexibility** as requirements change

Choose the configuration that best matches your current needs, and know that you can easily adapt as your application evolves.