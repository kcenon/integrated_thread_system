# 🚀 Integrated Thread System

> **A unified, high-performance threading framework that combines the power of thread_system, logger_system, and monitoring_system with the simplicity of a single-line initialization.**

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](LICENSE)
[![C++ Version](https://img.shields.io/badge/C%2B%2B-17%2F20-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](https://github.com/)

## 🎯 Why Integrated Thread System?

The Integrated Thread System recreates the **original simplicity** of the unified thread_system before it was split into three separate projects, while maintaining all the **advanced capabilities** gained from the separation.

### ✨ Key Features

- **🎮 One-Line Initialization** - Start immediately with zero configuration
- **⚡ Priority-Based Scheduling** - Critical, normal, and background job priorities
- **🧠 Adaptive Optimization** - Automatically switches between mutex and lock-free queues
- **📊 Real-Time Monitoring** - Built-in performance metrics and health checks
- **🔧 Production Ready** - Battle-tested components from three mature systems
- **💪 High Performance** - 1.16M+ jobs/second throughput capability

## 🚦 Quick Start

### Installation (30 seconds)

```bash
git clone https://github.com/your-org/integrated_thread_system
cd integrated_thread_system
./scripts/quick_start.sh dev
```

### Your First Program (1 minute)

```cpp
#include "unified_thread_system.h"
using namespace integrated_thread_system;

int main() {
    // One-line initialization!
    unified_thread_system system;

    // Submit a simple task
    auto future = system.submit([]() {
        return 42;
    });

    std::cout << "Answer: " << future.get() << std::endl;
    return 0;
}
```

### Priority-Based Processing (2 minutes)

```cpp
unified_thread_system system;

// Critical task - processed immediately
system.submit_critical([]() {
    handle_emergency();
});

// Normal priority - standard queue
system.submit([]() {
    process_request();
});

// Background - when resources available
system.submit_background([]() {
    cleanup_old_files();
});
```

## 📚 Documentation

### 🎓 Learning Path

| Level | Time | Start Here | Description |
|-------|------|------------|-------------|
| **Beginner** | 5 min | [Quick Start Guide](docs/getting_started/QUICK_START.md) | Get running in minutes |
| **Basic** | 15 min | [First Program](docs/getting_started/FIRST_PROGRAM.md) | Write your first application |
| **Intermediate** | 30 min | [Priority Scheduling](docs/guides/PRIORITY_SCHEDULING.md) | Use advanced scheduling |
| **Advanced** | 1 hour | [Performance Tuning](docs/advanced/PERFORMANCE_TUNING.md) | Optimize for production |

### 📖 Complete Documentation

- **[Getting Started](docs/getting_started/)** - Installation, quick start, FAQs
- **[User Guides](docs/guides/)** - Feature guides and patterns
- **[API Reference](docs/api/)** - Complete API documentation
- **[Architecture](docs/architecture/)** - Design and internals
- **[Examples](examples/)** - Runnable example programs

## 💡 Example Gallery

### Basic Examples
```cpp
// Hello Thread - Simplest possible example
auto result = system.submit([]() {
    return "Hello from thread!";
});
```

### Intermediate Examples
```cpp
// Batch processing with priorities
std::vector<task> tasks = load_tasks();
for (const auto& task : tasks) {
    if (task.is_urgent()) {
        system.submit_critical([task]() { process(task); });
    } else {
        system.submit([task]() { process(task); });
    }
}
```

### Advanced Examples
```cpp
// Custom priority types for business logic
enum class business_priority {
    customer_blocking,
    revenue_affecting,
    operational,
    maintenance
};

system.submit(business_priority::customer_blocking, []() {
    return handle_customer_issue();
});
```

### Real-World Scenarios
- [Web Server](examples/04_real_world/web_server.cpp) - HTTP request handling with priorities
- [Image Processor](examples/04_real_world/image_processor.cpp) - Thumbnail vs high-res processing
- [Database Pool](examples/04_real_world/database_pool.cpp) - Connection pool management
- [Message Queue](examples/04_real_world/message_queue.cpp) - Async message processing

## 🎯 Use Cases

### Web Services
```cpp
class web_handler {
    unified_thread_system system_;

    void handle_request(const request& req) {
        if (req.is_health_check()) {
            system_.submit_critical([req]() {
                return health_response();
            });
        } else {
            system_.submit([req]() {
                return process_request(req);
            });
        }
    }
};
```

### Data Processing
```cpp
class data_pipeline {
    unified_thread_system system_;

    void process_dataset(const dataset& data) {
        // Quick validation - high priority
        auto validation = system_.submit_critical([&data]() {
            return validate(data);
        });

        // Processing - normal priority
        auto processed = system_.submit([&data]() {
            return transform(data);
        });

        // Archiving - background
        system_.submit_background([&data]() {
            archive(data);
        });
    }
};
```

## 📊 Performance

### Benchmarks

| Metric | Performance | Conditions |
|--------|------------|------------|
| **Throughput** | 1.16M+ jobs/sec | 8 cores, high contention |
| **Latency** | < 100μs p99 | Normal priority jobs |
| **Memory** | 2MB base | 1000 job queue capacity |
| **Scaling** | Linear to 32 cores | CPU-bound workloads |

### Adaptive Optimization

The system automatically adapts to your workload:

- **Low Contention** → Mutex-based queue (lower overhead)
- **High Contention** → Lock-free queue (better scaling)
- **Mixed Workload** → Dynamic switching for optimal performance

## 🔧 Configuration

### Basic Configuration
```cpp
config cfg;
cfg.set_worker_count(8)
   .set_queue_capacity(10000)
   .enable_adaptive_optimization(true);

unified_thread_system system(cfg);
```

### Advanced Configuration
```cpp
config advanced;
advanced.set_worker_count(std::thread::hardware_concurrency())
        .enable_work_stealing(true)
        .enable_priority_inheritance(true)
        .set_monitoring_interval(std::chrono::seconds(1))
        .enable_performance_monitoring(true);

unified_thread_system system(advanced);
```

## 🛠️ Building from Source

### Prerequisites
- C++17 compatible compiler (GCC 8+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- Optional: vcpkg for dependency management

### Build Commands
```bash
# Quick build with automatic configuration
./scripts/build.sh --auto

# Manual build with options
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests
./scripts/test.sh --all

# Install
sudo make install
```

## 🧪 Testing

```bash
# Run all tests
./scripts/test.sh --all

# Run specific test categories
./scripts/test.sh --unit        # Unit tests only
./scripts/test.sh --integration # Integration tests
./scripts/test.sh --benchmark   # Performance benchmarks

# Run with sanitizers
./scripts/test.sh --sanitizers --memory
```

## 📦 Integration

### CMake Integration
```cmake
find_package(integrated_thread_system REQUIRED)
target_link_libraries(your_app
    PRIVATE
    integrated_thread_system::unified_thread_system
)
```

### Package Managers
```bash
# vcpkg
vcpkg install integrated-thread-system

# Conan
conan install integrated-thread-system/1.0.0@
```

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

### Areas for Contribution
- 🐛 Bug fixes and issue reports
- 📚 Documentation improvements
- 🎯 New example programs
- ⚡ Performance optimizations
- 🧪 Additional test coverage

## 📈 Roadmap

### Future Enhancements
- [ ] GPU task offloading support
- [ ] Distributed system support
- [ ] Advanced scheduling policies
- [ ] Machine learning workload optimization
- [ ] Cloud-native integrations
- [ ] Real-time system support

## 🙏 Acknowledgments

This project unifies three powerful systems:
- [thread_system](https://github.com/org/thread_system) - Core threading framework
- [logger_system](https://github.com/org/logger_system) - Asynchronous logging
- [monitoring_system](https://github.com/org/monitoring_system) - Performance monitoring

## 📄 License

BSD 3-Clause License - see [LICENSE](LICENSE) for details.

## 🆘 Support

- 📖 [Documentation](docs/)
- 💬 [Discussions](https://github.com/org/integrated_thread_system/discussions)
- 🐛 [Issue Tracker](https://github.com/org/integrated_thread_system/issues)
- 📧 Email: support@integrated-thread-system.org

---

<div align="center">

**[Quick Start](docs/getting_started/QUICK_START.md)** •
**[Examples](examples/)** •
**[API Docs](docs/api/)** •
**[Benchmarks](tests/benchmarks/)**

Made with ❤️ for the C++ community

</div>