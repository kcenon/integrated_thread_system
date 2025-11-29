# Troubleshooting Guide

**Version**: 1.0
**Last Updated**: 2025-11-30

---

## Build Issues

### CMake Configuration Fails

**Symptom**: CMake cannot find dependencies

**Solution**:
```bash
# Clean and reconfigure
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### C++20 Compilation Errors

**Symptom**: Compiler errors about C++20 features

**Solution**: Verify compiler version:
```bash
g++ --version  # Need GCC 10+
clang++ --version  # Need Clang 12+
```

Update CMake settings:
```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Linker Errors

**Symptom**: Undefined references

**Solution**: Ensure proper linking:
```cmake
target_link_libraries(your_target PRIVATE integrated_thread_system)
```

---

## Runtime Issues

### System Fails to Start

**Symptom**: `start()` returns error

**Solution**: Check configuration values:
```cpp
auto result = system->start();
if (!result) {
    std::cerr << "Error: " << result.error().message() << std::endl;
}
```

### High Memory Usage

**Symptom**: Memory grows continuously

**Solution**:
1. Check for job queue buildup
2. Verify job completion
3. Run with AddressSanitizer to detect leaks

```bash
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=address"
```

### Performance Degradation

**Symptom**: Lower throughput than expected

**Solution**:
1. Verify thread pool size matches CPU cores
2. Check for contention in job queues
3. Profile with perf or Instruments

```cpp
cfg.thread_pool_size = std::thread::hardware_concurrency();
```

---

## Thread Safety Issues

### Data Races

**Symptom**: Intermittent crashes or corruption

**Solution**: Run with ThreadSanitizer:
```bash
cmake -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread"
cmake --build build-tsan
./build-tsan/tests/unit_tests
```

### Deadlocks

**Symptom**: Application hangs

**Solution**:
1. Check for circular dependencies in jobs
2. Verify proper shutdown order
3. Use timeout in blocking operations

---

## Logging Issues

### Logs Not Appearing

**Symptom**: No log output

**Solution**:
1. Check log level configuration
2. Verify console logging is enabled
3. Check file permissions for file logging

```cpp
cfg.default_log_level = log_level::debug;
cfg.enable_console_logging = true;
```

### Log File Too Large

**Symptom**: Log files grow unbounded

**Solution**: Configure log rotation:
```cpp
cfg.max_log_file_size = 10 * 1024 * 1024;  // 10 MB
cfg.max_log_files = 5;
```

---

## Monitoring Issues

### Metrics Not Collected

**Symptom**: Empty metrics

**Solution**: Verify monitoring is enabled:
```cpp
cfg.enable_monitoring = true;
```

### High Monitoring Overhead

**Symptom**: CPU impact from monitoring

**Solution**: Enable adaptive monitoring:
```cpp
cfg.adaptive_monitoring = true;
```

---

## Getting Help

If issues persist:
1. Check [FAQ](FAQ.md)
2. Search [existing issues](https://github.com/kcenon/integrated_thread_system/issues)
3. Open a new issue with:
   - Platform and compiler info
   - Steps to reproduce
   - Error messages/logs
   - Minimal reproduction code
