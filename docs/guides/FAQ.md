# Frequently Asked Questions

**Version**: 1.0
**Last Updated**: 2025-11-30

---

## General Questions

### Q: What is integrated_thread_system?

A: integrated_thread_system is an enterprise-grade threading framework that combines thread_system, logger_system, and monitoring_system into a unified solution with zero-configuration setup.

### Q: What C++ standard is required?

A: C++20 is required. The system uses features like concepts, ranges, and std::jthread.

### Q: Which platforms are supported?

A: Linux (Ubuntu 20.04+), macOS (12+), and Windows (10+) are fully supported.

---

## Installation & Setup

### Q: How do I install the library?

A: The recommended method is CMake FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
    integrated_thread_system
    GIT_REPOSITORY https://github.com/kcenon/integrated_thread_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(integrated_thread_system)
```

### Q: What dependencies are required?

A: The library fetches its dependencies automatically:
- common_system
- thread_system
- logger_system
- monitoring_system

---

## Usage Questions

### Q: How do I start the system?

A: Simple initialization:

```cpp
auto system = integrated_thread_system::create();
system->start();
```

### Q: How do I configure the thread pool size?

A: Use the config struct:

```cpp
integrated_thread_system::config cfg;
cfg.thread_pool_size = 8;
auto system = integrated_thread_system::create(cfg);
```

### Q: How do I disable monitoring?

A: Set in configuration:

```cpp
cfg.enable_monitoring = false;
```

---

## Performance Questions

### Q: What throughput can I expect?

A: On modern hardware:
- Thread pool: 2.48M+ jobs/second
- Logging: 4.34M+ logs/second

### Q: What is the memory footprint?

A: Baseline memory is less than 5 MB for the full system.

---

## Troubleshooting

### Q: Build fails with C++20 errors

A: Ensure your compiler supports C++20:
- GCC 10+
- Clang 12+
- MSVC 2019 16.10+

### Q: Tests fail with sanitizer errors

A: Please report the issue with full sanitizer output to our GitHub issues.

---

## More Questions?

- Check [Troubleshooting](TROUBLESHOOTING.md)
- Open an [issue](https://github.com/kcenon/integrated_thread_system/issues)
