# Integration Guide

**Version**: 1.0
**Last Updated**: 2025-11-30

---

## Overview

This guide covers integrating integrated_thread_system with other systems and applications.

---

## CMake Integration

### FetchContent (Recommended)

```cmake
include(FetchContent)

FetchContent_Declare(
    integrated_thread_system
    GIT_REPOSITORY https://github.com/kcenon/integrated_thread_system.git
    GIT_TAG main
)

FetchContent_MakeAvailable(integrated_thread_system)

target_link_libraries(your_target PRIVATE integrated_thread_system)
```

### Subdirectory

```cmake
add_subdirectory(external/integrated_thread_system)
target_link_libraries(your_target PRIVATE integrated_thread_system)
```

### find_package

```cmake
find_package(integrated_thread_system REQUIRED)
target_link_libraries(your_target PRIVATE integrated_thread_system::integrated_thread_system)
```

---

## Integration with Subsystems

### Using Individual Components

```cpp
// Access thread pool directly
auto pool = system->get_thread_pool();
pool->submit([]() { /* work */ });

// Access logger directly
auto logger = system->get_logger();
logger->info("message");

// Access monitor directly
auto monitor = system->get_monitor();
monitor->increment("counter");
```

### Custom Configuration

```cpp
integrated_thread_system::config cfg;

// Customize thread pool
cfg.thread_pool_size = 16;
cfg.enable_work_stealing = true;

// Customize logging
cfg.default_log_level = log_level::debug;
cfg.log_file_path = "/var/log/myapp.log";

// Customize monitoring
cfg.enable_monitoring = true;
cfg.adaptive_monitoring = true;

auto system = integrated_thread_system::create(cfg);
```

---

## Related Documentation

- [Quick Start](../guides/QUICK_START.md)
- [API Reference](../API_REFERENCE.md)
- [Architecture](../ARCHITECTURE.md)
