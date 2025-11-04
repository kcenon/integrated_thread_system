# Troubleshooting Guide

This guide helps you diagnose and resolve common issues when building, integrating, or using the Integrated Thread System.

## Table of Contents

- [Build Issues](#build-issues)
- [Dependency Problems](#dependency-problems)
- [Runtime Errors](#runtime-errors)
- [Performance Issues](#performance-issues)
- [Platform-Specific Issues](#platform-specific-issues)
- [Integration Issues](#integration-issues)

---

## Build Issues

### CMake cannot find dependencies

**Symptom**:
```
CMake Error: Could not find thread_system
CMake Error: Could not find common_system
```

**Solutions**:

1. **Check dependency locations**:
   ```bash
   # CMake searches in this order:
   ls $HOME/Sources/thread_system       # User home (developer priority)
   ls ../thread_system                  # Sibling directory (CI priority)
   ls /usr/local/include/thread_system  # System installation
   ```

2. **Let FetchContent handle it**:
   ```bash
   # CMake will automatically download from GitHub if not found locally
   cmake -B build -DUSE_SYSTEM_DEPENDENCIES=OFF
   ```

3. **Manual dependency installation**:
   ```bash
   cd ~/Sources
   git clone https://github.com/kcenon/common_system.git
   git clone https://github.com/kcenon/thread_system.git
   git clone https://github.com/kcenon/logger_system.git
   git clone https://github.com/kcenon/monitoring_system.git

   # Build each dependency
   for dir in thread_system logger_system monitoring_system; do
       cd $dir && cmake -B build && cmake --build build && cd ..
   done
   ```

---

### C++20 not supported

**Symptom**:
```
CMake Error: The C++ compiler does not support C++20
error: 'std::jthread' has not been declared
```

**Solution**:

Upgrade to a C++20-compatible compiler:

| Platform | Minimum Version | Recommended |
|----------|----------------|-------------|
| GCC | 11+ | 13+ (full std::format support) |
| Clang | 14+ | 15+ (full std::format support) |
| MSVC | 19.29+ (VS 2019 16.10+) | Latest |
| Apple Clang | 14+ (Xcode 14+) | Latest |

**Ubuntu**:
```bash
sudo apt update
sudo apt install gcc-13 g++-13
export CXX=g++-13
cmake -B build
```

**macOS**:
```bash
xcode-select --install  # Install Xcode Command Line Tools
# Or install full Xcode from App Store
```

**Windows**:
- Install Visual Studio 2019 16.10+ or Visual Studio 2022

---

### Linker errors with external systems

**Symptom**:
```
undefined reference to `kcenon::thread::thread_pool::execute(...)`
undefined reference to `kcenon::logger::logger::write(...)`
```

**Solutions**:

1. **Ensure dependencies are built**:
   ```bash
   # Check if libraries exist
   ls ~/Sources/thread_system/build/lib/libthread_system.a
   ls ~/Sources/logger_system/build/lib/liblogger_system.a
   ls ~/Sources/monitoring_system/build/lib/libmonitoring_system.a
   ```

2. **Rebuild dependencies**:
   ```bash
   cd ~/Sources/thread_system
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

3. **Use header-only mode** (if libraries unavailable):
   ```bash
   cmake -B build -DTHREAD_SYSTEM_HEADER_ONLY=ON
   ```

---

### vcpkg integration issues

**Symptom**:
```
Could not find package nlohmann_json
```

**Solution**:

1. **Install vcpkg**:
   ```bash
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh  # or ./bootstrap-vcpkg.bat on Windows
   ```

2. **Integrate with CMake**:
   ```bash
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

3. **Or install dependencies manually**:
   ```bash
   ./vcpkg install nlohmann-json gtest benchmark
   ```

---

## Dependency Problems

### Version mismatch

**Symptom**:
```
CMake Error: common_system version 0.9.0 is too old. Required: 1.0.0 or newer
```

**Solution**:

Update the dependency to the required version:

```bash
cd ~/Sources/common_system
git fetch --tags
git checkout v1.0.0  # or latest compatible version

# For thread_system
cd ~/Sources/thread_system
git fetch --tags
git checkout v2.0.0

# For logger_system
cd ~/Sources/logger_system
git fetch --tags
git checkout v1.0.0

# For monitoring_system
cd ~/Sources/monitoring_system
git fetch --tags
git checkout v1.0.0
```

---

### Conflicting header versions

**Symptom**:
```
error: 'Result' is ambiguous
error: redefinition of 'struct kcenon::common::Result'
```

**Solution**:

Ensure all dependencies use compatible versions:

```bash
# Check versions
cd ~/Sources/common_system && git describe --tags
cd ~/Sources/thread_system && git describe --tags
cd ~/Sources/logger_system && git describe --tags
cd ~/Sources/monitoring_system && git describe --tags

# Use the version compatibility matrix from DEPENDENCIES.md
```

Refer to [DEPENDENCIES.md](DEPENDENCIES.md) for the compatibility matrix.

---

## Runtime Errors

### Thread pool initialization failure

**Symptom**:
```
terminate called after throwing an instance of 'std::runtime_error'
  what():  Failed to initialize thread pool
```

**Causes and Solutions**:

1. **Insufficient system resources**:
   ```cpp
   // Reduce thread count
   config cfg;
   cfg.thread_count = 4;  // Instead of default (hardware_concurrency)
   unified_thread_system system(cfg);
   ```

2. **Invalid configuration**:
   ```cpp
   // Check configuration validity
   config cfg;
   cfg.thread_count = 0;  // 0 = auto-detect (valid)
   cfg.max_queue_size = 10000;  // Must be > 0
   ```

3. **Check error details**:
   ```cpp
   try {
       unified_thread_system system(cfg);
   } catch (const std::exception& e) {
       std::cerr << "Initialization failed: " << e.what() << std::endl;
       // Check error message for specific component that failed
   }
   ```

---

### Log files not created

**Symptom**:
No log files appear in the configured directory.

**Solutions**:

1. **Check directory permissions**:
   ```bash
   # Ensure log directory is writable
   mkdir -p ./logs
   chmod 755 ./logs
   ```

2. **Verify logging is enabled**:
   ```cpp
   config cfg;
   cfg.enable_file_logging = true;  // Must be true
   cfg.log_directory = "./logs";    // Ensure directory exists
   unified_thread_system system(cfg);
   ```

3. **Check if logger initialized**:
   ```cpp
   // Enable console logging to see errors
   config cfg;
   cfg.enable_console_logging = true;
   cfg.enable_file_logging = true;
   ```

---

### Tasks not executing

**Symptom**:
Submitted tasks never complete, `future.get()` hangs indefinitely.

**Causes and Solutions**:

1. **Thread pool not initialized**:
   ```cpp
   // Ensure system is constructed before submitting tasks
   unified_thread_system system;  // Must complete before submit()
   auto future = system.submit([]() { return 42; });
   ```

2. **Deadlock in task**:
   ```cpp
   // Avoid waiting for futures within tasks submitted to the same pool
   // BAD:
   system.submit([&system]() {
       auto inner_future = system.submit([]() { return 1; });
       return inner_future.get();  // May deadlock if pool is full
   });

   // GOOD: Use separate pools or ensure sufficient workers
   config cfg;
   cfg.thread_count = std::max(8u, std::thread::hardware_concurrency());
   unified_thread_system system(cfg);
   ```

3. **Check system health**:
   ```cpp
   auto health = system.get_health();
   if (health.overall_health != health_level::healthy) {
       std::cerr << "System health issues:" << std::endl;
       for (const auto& issue : health.issues) {
           std::cerr << "  - " << issue << std::endl;
       }
   }
   ```

---

### Memory leaks detected

**Symptom**:
```
==12345==ERROR: LeakSanitizer: detected memory leaks
```

**Solutions**:

1. **Ensure proper shutdown**:
   ```cpp
   {
       unified_thread_system system;
       // Submit tasks...
       system.wait_for_completion();  // Wait before destruction
   }  // Destructor calls shutdown automatically
   ```

2. **Check for circular references**:
   ```cpp
   // BAD: Capturing shared_ptr to system in task
   auto system_ptr = std::make_shared<unified_thread_system>();
   system_ptr->submit([system_ptr]() {  // Circular reference!
       // ...
   });

   // GOOD: Use weak_ptr or don't capture system
   auto system_ptr = std::make_shared<unified_thread_system>();
   system_ptr->submit([]() {
       // No capture of system_ptr
   });
   ```

3. **Verify with sanitizers**:
   ```bash
   cmake -B build -DENABLE_SANITIZERS=ON
   cmake --build build
   ./build/your_app
   ```

---

## Performance Issues

### Lower than expected throughput

**Symptom**:
Achieving < 500K tasks/sec when documentation claims 1.16M+ tasks/sec.

**Solutions**:

1. **Check build configuration**:
   ```bash
   # Ensure Release build
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

2. **Enable work stealing**:
   ```cpp
   config cfg;
   cfg.enable_work_stealing = true;  // Should be true by default
   ```

3. **Optimize thread count**:
   ```cpp
   // For CPU-bound tasks
   config cfg;
   cfg.thread_count = std::thread::hardware_concurrency();

   // For I/O-bound tasks
   config cfg;
   cfg.thread_count = std::thread::hardware_concurrency() * 2;
   ```

4. **Minimize task overhead**:
   ```cpp
   // BAD: Submitting tiny tasks
   for (int i = 0; i < 1000000; ++i) {
       system.submit([i]() { return i; });  // Too much overhead
   }

   // GOOD: Batch processing
   std::vector<int> data(1000000);
   std::iota(data.begin(), data.end(), 0);
   auto futures = system.submit_batch(data.begin(), data.end(),
       [](int i) { return process(i); });
   ```

5. **Use external systems**:
   ```bash
   # Ensure EXTERNAL_SYSTEMS_AVAILABLE is defined
   cmake -B build -DEXTERNAL_SYSTEMS_AVAILABLE=ON
   ```

---

### High latency

**Symptom**:
P99 latency > 100μs for simple tasks.

**Solutions**:

1. **Check metrics**:
   ```cpp
   auto metrics = system.get_metrics();
   std::cout << "Queue size: " << metrics.queue_size << std::endl;
   std::cout << "Queue utilization: " << metrics.queue_utilization_percent << "%" << std::endl;
   ```

2. **Increase queue size if saturated**:
   ```cpp
   config cfg;
   cfg.max_queue_size = 100000;  // Increase from default 10000
   ```

3. **Use priority scheduling for critical tasks**:
   ```cpp
   // Submit critical tasks first
   auto critical = system.submit_critical([]() { /* urgent work */ });
   ```

4. **Reduce logging overhead**:
   ```cpp
   config cfg;
   cfg.enable_console_logging = false;  // Console I/O is slow
   cfg.enable_file_logging = true;      // Async file logging is faster
   ```

---

### Memory usage growing unbounded

**Symptom**:
Memory usage continuously increases during operation.

**Solutions**:

1. **Limit metrics storage**:
   ```cpp
   config cfg;
   cfg.monitoring.max_samples_per_metric = 1000;  // Reduce from 10000
   ```

2. **Check for task backlog**:
   ```cpp
   auto metrics = system.get_metrics();
   if (metrics.queue_size > 10000) {
       std::cerr << "Warning: Large queue backlog ("
                 << metrics.queue_size << " tasks)" << std::endl;
       // Reduce submission rate or increase workers
   }
   ```

3. **Ensure tasks complete**:
   ```cpp
   // Periodically check for stuck tasks
   auto health = system.get_health();
   if (health.queue_utilization_percent > 90.0) {
       std::cerr << "Warning: Queue nearly full" << std::endl;
   }
   ```

---

## Platform-Specific Issues

### Windows: File path issues

**Symptom**:
```
Failed to create log file: C:\Users\...\logs\app.log
```

**Solution**:

Use forward slashes or escape backslashes:

```cpp
config cfg;
cfg.log_directory = "C:/Users/YourName/logs";  // Forward slashes work on Windows
// OR
cfg.log_directory = "C:\\Users\\YourName\\logs";  // Escaped backslashes
```

---

### macOS: System metrics unavailable

**Symptom**:
CPU/memory usage always shows 0% on macOS.

**Explanation**:
System metrics use different APIs on macOS. Ensure `monitoring_system` is built with macOS support.

**Solution**:

Rebuild monitoring_system:

```bash
cd ~/Sources/monitoring_system
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

### Linux: Permission denied errors

**Symptom**:
```
Permission denied when accessing /proc/stat
```

**Solution**:

Ensure process has permission to read system stats:

```bash
# Check permissions
ls -l /proc/stat
# Should be readable by all (r--r--r--)

# If running in container, ensure proper capabilities
docker run --cap-add=SYS_ADMIN your_container
```

---

## Integration Issues

### Cannot link with other thread pools

**Symptom**:
```
error: multiple definitions of operator new
```

**Cause**:
Memory pool conflicts between different threading libraries.

**Solution**:

Use `integrated_thread_system` exclusively, or ensure proper namespace isolation:

```cpp
// Isolate in namespace
namespace my_app {
    kcenon::integrated::unified_thread_system thread_system;
}

namespace other_lib {
    external_thread_pool other_pool;
}
```

---

### Conflicts with existing logging

**Symptom**:
Logs appear duplicated or in wrong format.

**Solution**:

Disable integrated logging if you have your own:

```cpp
config cfg;
cfg.enable_file_logging = false;
cfg.enable_console_logging = false;
unified_thread_system system(cfg);

// Use your own logging
external_logger::log("Using external logger");
```

---

### Prometheus metrics not exporting

**Symptom**:
`export_metrics_prometheus()` returns empty string.

**Solutions**:

1. **Ensure monitoring is enabled**:
   ```cpp
   config cfg;
   cfg.monitoring.enable_profiling = true;
   cfg.monitoring.enable_system_monitoring = true;
   ```

2. **Submit some tasks first**:
   ```cpp
   // Metrics are only available after tasks execute
   system.submit([]() { /* do work */ }).get();

   // Now export metrics
   std::string prometheus = system.export_metrics_prometheus();
   ```

3. **Check for external system availability**:
   ```cpp
   #ifdef EXTERNAL_SYSTEMS_AVAILABLE
       auto prometheus = system.export_metrics_prometheus();
   #else
       // Fallback implementation may have limited metrics
   #endif
   ```

---

## Getting Help

If your issue is not covered here:

1. **Check GitHub Issues**: [integrated_thread_system/issues](https://github.com/kcenon/integrated_thread_system/issues)
2. **Search Documentation**: See [docs/](docs/) directory
3. **Enable Debug Logging**:
   ```cpp
   config cfg;
   cfg.min_log_level = log_level::debug;  // or log_level::trace
   cfg.enable_console_logging = true;
   ```
4. **File a Bug Report**: Include:
   - Platform (OS, compiler, version)
   - Build configuration (CMake flags)
   - Minimal reproducible example
   - Error messages and stack traces

---

## See Also

- [DEPENDENCIES.md](DEPENDENCIES.md) - Dependency requirements and troubleshooting
- [README.md](README.md) - Project overview and quick start
- [docs/API.md](docs/API.md) - Complete API reference
- [MIGRATION.md](MIGRATION.md) - Migration guide from individual systems
