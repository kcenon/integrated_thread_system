# Monitoring Examples

This directory contains examples demonstrating the monitoring features integrated from monitoring_system v4.0.0.

## Examples Overview

| Example | Description | Features |
|---------|-------------|----------|
| `adaptive_monitoring_demo.cpp` | Load-based adaptive sampling | Adaptive monitoring, system resources |
| `health_check_demo.cpp` | Health status aggregation | Health checks, Kubernetes probes |

## Quick Start

```bash
# Build monitoring examples
cmake --build build --target adaptive_monitoring_demo health_check_demo

# Run examples
./build/examples/monitoring/adaptive_monitoring_demo
./build/examples/monitoring/health_check_demo
```

## adaptive_monitoring_demo.cpp

Demonstrates **adaptive monitoring** that automatically adjusts sampling based on system load.

### Key Features

- **Load-based sampling** - Reduces sampling rate under high CPU load
- **Automatic interval adaptation** - Adjusts collection intervals dynamically
- **Scoped operation timing** - Track operation durations with RAII
- **System resource collection** - CPU, memory, disk, network metrics

### Code Highlights

```cpp
// Configure adaptive monitoring
monitoring_config config;
config.enable_adaptive_monitoring = true;
config.adaptive_low_threshold = 0.3;   // Increase sampling below 30%
config.adaptive_high_threshold = 0.7;  // Decrease sampling above 70%
config.adaptive_min_interval = std::chrono::milliseconds(100);
config.adaptive_max_interval = std::chrono::milliseconds(5000);

monitoring_adapter monitor(config);

// Scoped operation timing
auto timer = monitor.time_operation("workload.process");
// ... do work ...
if (operation_failed) {
    timer.mark_failed();
}
```

### Expected Output

```
=== Adaptive Monitoring Demo (monitoring_system v4.0.0) ===

--- Phase 1: Normal Load (5 seconds) ---
=== Adaptation Statistics ===
Total adaptations: 2
Current sampling rate: 1.00
Current interval: 500ms
Average CPU usage: 25.3%

--- Phase 2: High Load (5 seconds) ---
Starting CPU-intensive tasks...
=== Adaptation Statistics ===
Total adaptations: 5
Current sampling rate: 0.50
Current interval: 1000ms
Average CPU usage: 95.2%
```

## health_check_demo.cpp

Demonstrates **health monitoring** with custom health checks and Kubernetes-style probes.

### Key Features

- **Custom health checks** - Register checks for your components
- **Automatic aggregation** - Combine multiple check results
- **Status levels** - HEALTHY, DEGRADED, UNHEALTHY
- **Kubernetes probes** - Liveness and readiness patterns

### Code Highlights

```cpp
// Register health checks
monitor.register_health_check("database", [&db]() {
    return db.ping();
});

monitor.register_health_check("cache", [&cache]() {
    return cache.is_healthy();
});

// Composite health check
monitor.register_health_check("critical_path", [&db, &mq]() {
    return db.ping() && mq.is_healthy();
});

// Check overall health
auto health = monitor.check_health();
if (health.is_ok()) {
    switch (health.value().status) {
        case health_status::healthy: /* all good */ break;
        case health_status::degraded: /* some issues */ break;
        case health_status::unhealthy: /* critical */ break;
    }
}
```

### Kubernetes-Style Probes

```cpp
// Liveness probe - is the application alive?
auto liveness = [&monitor]() -> bool {
    auto result = monitor.check_health();
    return result.is_ok() &&
           result.value().status != health_status::unhealthy;
};

// Readiness probe - ready to serve traffic?
auto readiness = [&monitor]() -> bool {
    auto result = monitor.check_health();
    return result.is_ok() &&
           result.value().status == health_status::healthy;
};
```

### Expected Output

```
=== Health Check Demo (monitoring_system v4.0.0) ===

--- Phase 1: All Components Starting ---
=== Health Check Result ===
Status: HEALTHY
Message: All checks passed

--- Phase 2: Database Failure ---
=== Health Check Result ===
Status: UNHEALTHY
Message: 2 checks failed
Component status:
  database = FAILED
  critical_path = FAILED
```

## Configuration Reference

### monitoring_config

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enable_monitoring` | bool | true | Enable metrics collection |
| `enable_adaptive_monitoring` | bool | false | Enable load-based adaptation |
| `enable_health_monitoring` | bool | false | Enable health checks |
| `enable_system_resource_collector` | bool | false | Collect CPU/memory/etc |
| `sampling_interval` | duration | 1000ms | Base collection interval |
| `cpu_threshold` | double | 80.0 | CPU warning threshold (%) |
| `memory_threshold` | double | 90.0 | Memory warning threshold (%) |
| `health_check_interval` | duration | 5000ms | Health check frequency |

## API Reference

| Class | Description |
|-------|-------------|
| `monitoring_adapter` | Main monitoring interface |
| `monitoring_config` | Configuration options |
| `scoped_timer` | RAII operation timing |
| `health_check_result` | Health status information |
| `adaptation_stats` | Adaptive monitoring statistics |

## Related Examples

- [Basic Examples](../01_basic/) - Getting started
- [Advanced Examples](../03_advanced/) - Complex patterns
- [System Combinations](../05_system_combinations/) - Multi-system integration

## See Also

- [monitoring_system v4.0.0 Documentation](https://github.com/kcenon/monitoring_system)
- [API Reference](../../docs/API.md)
- [Main README](../../README.md)
