# Integrated Thread System

[![CodeFactor](https://www.codefactor.io/repository/github/kcenon/integrated_thread_system/badge)](https://www.codefactor.io/repository/github/kcenon/integrated_thread_system)

[![Ubuntu-GCC](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-gcc.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-gcc.yaml)
[![Ubuntu-Clang](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-clang.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-clang.yaml)
[![Windows-VS](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-vs.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-vs.yaml)
[![Windows-MSYS2](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-msys2.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-msys2.yaml)
[![Doxygen](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-Doxygen.yaml)

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)

## 📖 Documentation

- **[📚 Complete Documentation](docs/)**: Comprehensive documentation organized by category
- **[🔗 API Documentation](https://kcenon.github.io/integrated_thread_system/)**: Auto-generated Doxygen documentation
- **[🏗️ Architecture](docs/architecture/)**: System design and architectural documentation
- **[🔧 Development Guides](docs/development/)**: Build scripts and integration guides
- **[🚀 Getting Started](docs/getting_started/)**: Quick start tutorials and basic setup
- **[📖 User Guides](docs/guides/)**: Detailed usage guides and best practices

## 🌟 Overview

The **Integrated Thread System** provides a unified, zero-configuration threading framework with built-in logging and monitoring. It combines the power of three enterprise-grade systems into a single, easy-to-use API that matches the simplicity of the original thread_system while offering advanced features.

### 🚀 Key Features

#### **Standard Version** (`integrated_thread_system`)
- ✨ **Zero-Configuration**: Works out of the box with sensible defaults
- 🧵 **Thread Pool Management**: Automatic worker scaling and work stealing
- 📝 **Integrated Logging**: Built-in file and console logging
- 📊 **Performance Monitoring**: Real-time metrics and health checks
- 🎯 **Simple API**: Intuitive interface matching original thread_system

#### **Enhanced Version** (`integrated_thread_system_enhanced`)
- 🎯 **Priority Scheduling**: Critical, normal, and background task priorities
- ❌ **Cancellation Tokens**: Safe task cancellation support
- ⏰ **Scheduled Execution**: Delayed and recurring task execution
- 🔄 **Map-Reduce Pattern**: Built-in parallel data processing
- 🛡️ **Circuit Breaker**: Automatic failure isolation
- 📡 **Event System**: Publish-subscribe event handling
- 🔌 **Plugin Support**: Dynamic feature extension

### 🔗 Core Components Integration
- **[thread_system](https://github.com/kcenon/thread_system)**: High-performance threading (1.16M+ jobs/sec)
- **[logger_system](https://github.com/kcenon/logger_system)**: Asynchronous structured logging
- **[monitoring_system](https://github.com/kcenon/monitoring_system)**: Comprehensive observability

## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/kcenon/integrated_thread_system.git
cd integrated_thread_system

# Build with automatic configuration
./build.sh --clean
```

### Basic Usage

```cpp
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

int main() {
    // Zero-configuration setup
    unified_thread_system system;

    // Submit a simple task
    auto future = system.submit([]() {
        return 42;
    });

    // Get the result
    std::cout << "Result: " << future.get() << std::endl;

    return 0;
}
```

### Enhanced Features Usage

```cpp
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

int main() {
    // Configure enhanced features
    config cfg;
    cfg.enable_circuit_breaker = true;
    cfg.enable_work_stealing = true;

    unified_thread_system system(cfg);

    // Priority-based task submission
    auto critical = system.submit_critical([]() {
        return "Critical task";
    });

    // Cancellable task
    cancellation_token token;
    auto cancellable = system.submit_cancellable(token, []() {
        return "Can be cancelled";
    });
    token.cancel();  // Cancel the task

    // Scheduled task (100ms delay)
    auto scheduled = system.schedule(100ms, []() {
        return "Delayed execution";
    });

    // Map-Reduce pattern
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto result = system.map_reduce(
        data.begin(), data.end(),
        [](int n) { return n * n; },     // Map: square
        [](int a, int b) { return a + b; }, // Reduce: sum
        0
    );

    // Performance metrics
    auto metrics = system.get_metrics();
    std::cout << "Tasks completed: " << metrics.tasks_completed << std::endl;
    std::cout << "Average latency: " << metrics.average_latency.count() << "ns" << std::endl;

    return 0;
}
```

## 📊 Performance Metrics

The system provides comprehensive performance monitoring:

```cpp
struct performance_metrics {
    size_t tasks_submitted;
    size_t tasks_completed;
    size_t tasks_failed;
    size_t tasks_cancelled;

    // Latency metrics
    std::chrono::nanoseconds average_latency;
    std::chrono::nanoseconds min_latency;
    std::chrono::nanoseconds max_latency;
    std::chrono::nanoseconds p95_latency;
    std::chrono::nanoseconds p99_latency;

    // System metrics
    size_t active_workers;
    size_t queue_size;
    double queue_utilization_percent;
    double tasks_per_second;
};
```

## 🏥 Health Monitoring

Real-time health status with automatic issue detection:

```cpp
struct health_status {
    health_level overall_health;  // healthy, degraded, critical, failed
    double cpu_usage_percent;
    double memory_usage_percent;
    double queue_utilization_percent;
    bool circuit_breaker_open;
    size_t consecutive_failures;
    std::vector<std::string> issues;
};
```

## 🔧 Configuration Options

```cpp
struct config {
    std::string name = "ThreadSystem";
    size_t thread_count = 0;  // 0 = auto-detect

    // Logging
    bool enable_file_logging = true;
    bool enable_console_logging = true;
    std::string log_directory = "./logs";
    log_level min_log_level = log_level::info;

    // Enhanced features
    bool enable_circuit_breaker = false;
    size_t circuit_breaker_failure_threshold = 5;
    std::chrono::milliseconds circuit_breaker_reset_timeout{5000};
    size_t max_queue_size = 10000;
    bool enable_work_stealing = true;
    bool enable_dynamic_scaling = false;
    size_t min_threads = 1;
    size_t max_threads = 0;  // 0 = no limit
};
```

## 📦 Build Options

```bash
# Standard build
./build.sh

# Clean build with all features
./build.sh --clean

# Build with tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Build with examples
cmake -B build -DBUILD_EXAMPLES=ON
cmake --build build

# Build with specific compiler
./build.sh --compiler clang++
```

## 🧪 Testing

The project includes comprehensive test coverage:

```bash
# Run all tests
./build/tests/unit/test_basic_operations

# Run enhanced features test
g++ -std=c++17 -I./include -o test_enhanced examples/test_enhanced.cpp \
    build/libintegrated_thread_system_enhanced.a -pthread
./test_enhanced
```

## 📈 Benchmarks

Performance characteristics:
- **Task Throughput**: 1.16M+ tasks/second
- **Latency**: Sub-microsecond for simple tasks
- **Scalability**: Linear scaling up to CPU core count
- **Memory**: Minimal overhead with pooled allocations

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## 📄 License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

This project integrates and builds upon:
- [thread_system](https://github.com/kcenon/thread_system)
- [logger_system](https://github.com/kcenon/logger_system)
- [monitoring_system](https://github.com/kcenon/monitoring_system)

## 📬 Contact

- **Author**: Neowine
- **Email**: kcenon@gmail.com
- **GitHub**: [@kcenon](https://github.com/kcenon)