# 🚀 Integrated Thread System

<div align="center">

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)

[![Ubuntu-GCC](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-gcc.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-gcc.yaml)
[![Ubuntu-Clang](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-clang.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-clang.yaml)
[![Windows-VS](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-vs.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-vs.yaml)
[![Windows-MSYS2](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-msys2.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-msys2.yaml)
[![Doxygen](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-Doxygen.yaml)

**Enterprise-Grade Threading Framework with Zero-Configuration Setup**

[📚 Documentation](docs/) | [🔗 API Reference](https://kcenon.github.io/integrated_thread_system/) | [📖 Examples](docs/EXAMPLES.md) | [📊 Benchmarks](#-performance-benchmarks) | [🔄 Changelog](CHANGELOG.md)

</div>

---

## 🔗 Project Ecosystem & Inter-Dependencies

This project is the **unified integration** of three specialized, battle-tested systems, creating a complete threading solution with enterprise-grade observability and reliability.

### Core Components Architecture

```
┌─────────────────────────────────────────────────────────┐
│         integrated_thread_system (This Project)         │
│              Unified Threading Solution                 │
└──────────────┬──────────────────────┬───────────────────┘
               │                      │
    ┌──────────▼──────────┐  ┌────────▼──────────┐
    │   thread_system     │  │  logger_system    │
    │ High-perf threading │  │  Async logging    │
    └──────────┬──────────┘  └────────┬──────────┘
               │                      │
               │  ┌────────────────────▼──────────┐
               └──►  monitoring_system            │
                  │  Real-time metrics & health   │
                  └───────────────────────────────┘
                               │
                  ┌────────────▼────────────┐
                  │     common_system       │
                  │  Foundation interfaces  │
                  └─────────────────────────┘
```

### Component Integration

#### 1. **[thread_system](https://github.com/kcenon/thread_system)** - Concurrency Engine
**What it provides:**
- Lock-free thread pools (2.48M+ jobs/sec throughput)
- Priority-based scheduling (RealTime, Batch, Background)
- Adaptive job queues with automatic optimization
- Hazard pointer memory reclamation
- Work-stealing architecture

**Integration:** Core threading primitives that power all async operations

#### 2. **[logger_system](https://github.com/kcenon/logger_system)** - Observability
**What it provides:**
- Asynchronous logging (4.34M+ logs/sec)
- Multiple output targets (console, file, network)
- Sub-microsecond latency (148ns average)
- Structured logging with JSON support
- Thread-safe by design

**Integration:** Automatic logging for thread pool operations, job execution, and errors

#### 3. **[monitoring_system](https://github.com/kcenon/monitoring_system)** - Telemetry
**What it provides:**
- Real-time performance metrics
- Health check aggregation
- Prometheus/JSON export
- System resource monitoring (CPU, memory)
- Custom metric collection

**Integration:** Live dashboards showing thread pool utilization, throughput, and system health

#### 4. **[common_system](https://github.com/kcenon/common_system)** - Foundation
**What it provides:**
- `Result<T>` error handling pattern
- `IExecutor` interface abstraction
- Centralized error code registry
- Cross-platform utilities
- Design pattern implementations

**Integration:** Foundation layer ensuring consistent interfaces and error handling

### Dependency Flow

```
common_system (interfaces & patterns)
    ↓ provides foundation
thread_system (core threading)
    ↓ uses for execution
logger_system (observability) + monitoring_system (telemetry)
    ↓ integrates all three
integrated_thread_system (unified solution)
```

### Integration Benefits

- **Zero-Configuration Setup**: Works out of the box with smart defaults
- **Automatic Observability**: Logging and monitoring integrated by design
- **Consistent Error Handling**: Unified Result<T> pattern across all components
- **Performance Optimized**: Each system fine-tuned for its domain
- **Pluggable Architecture**: Use only the components you need

> 📖 **[Complete Ecosystem Guide](docs/ECOSYSTEM.md)**: Detailed documentation of system interactions, integration patterns, and advanced usage scenarios.

---

## ✨ Why Integrated Thread System?

Modern C++ applications need more than just thread pools. They need **unified solutions** for concurrent programming with built-in logging, monitoring, and error handling. This framework delivers exactly that - combining three battle-tested systems into one coherent, easy-to-use package.

### 🎯 Key Benefits

<table>
<tr>
<td width="50%">

**🚅 Blazing Fast**
- 1.16M+ tasks/second throughput
- Sub-microsecond latency
- Zero-allocation fast paths
- Lock-free queues where possible

</td>
<td width="50%">

**🛡️ Production Ready**
- Battle-tested in enterprise environments
- Comprehensive error handling
- Circuit breaker patterns
- Automatic failure recovery

</td>
</tr>
<tr>
<td width="50%">

**📊 Observable by Default**
- Real-time performance metrics
- Health monitoring
- Distributed tracing support
- Prometheus/JSON export

</td>
<td width="50%">

**🔧 Zero Configuration**
- Works out of the box
- Smart defaults
- Auto-tuning
- No boilerplate

</td>
</tr>
</table>

## 🌟 Quick Start

### 30-Second Setup

```bash
# Clone and build
git clone https://github.com/kcenon/integrated_thread_system.git
cd integrated_thread_system
./build.sh --clean

# Run example
./build/examples/hello_thread
```

### Your First Program

```cpp
#include <kcenon/integrated/unified_thread_system.h>
using namespace kcenon::integrated;

int main() {
    unified_thread_system system;  // That's it! Zero configuration

    auto result = system.submit([]() {
        return "Hello from " + std::to_string(std::this_thread::get_id());
    }).get();

    std::cout << result << std::endl;
}
```

## 📦 Two Flavors: Choose Your Power Level

<table>
<tr>
<th width="50%">Standard Edition</th>
<th width="50%">Enhanced Edition</th>
</tr>
<tr>
<td>

Perfect for most applications

```cpp
unified_thread_system system;

// Simple task submission
auto future = system.submit(task);

// Batch processing
auto results = system.submit_batch(
    data.begin(), data.end(), processor
);

// Basic monitoring
auto metrics = system.get_metrics();
```

✅ Thread pool management
✅ Integrated logging
✅ Performance monitoring
✅ Health checks
✅ Work stealing

</td>
<td>

Advanced features for complex systems

```cpp
config cfg;
cfg.enable_circuit_breaker = true;
unified_thread_system system(cfg);

// Priority scheduling
system.submit_critical(urgent_task);

// Cancellable operations
cancellation_token token;
system.submit_cancellable(token, task);

// Map-Reduce patterns
system.map_reduce(begin, end,
    mapper, reducer, initial);
```

✅ Everything in Standard, plus:
✅ Priority scheduling
✅ Cancellation tokens
✅ Scheduled/recurring tasks
✅ Circuit breaker
✅ Event system
✅ Plugin architecture

</td>
</tr>
</table>

## 🔥 Real-World Examples

### Web Server with Priority Handling
```cpp
class WebServer {
    unified_thread_system system_;

public:
    auto handle_request(const Request& req) {
        // Health checks are critical priority
        if (req.path == "/health") {
            return system_.submit_critical([]() {
                return Response{200, "OK"};
            });
        }

        // API calls are normal priority
        if (req.path.starts_with("/api/")) {
            return system_.submit([req]() {
                return process_api(req);
            });
        }

        // Analytics are background priority
        return system_.submit_background([req]() {
            return process_analytics(req);
        });
    }
};
```

### Data Pipeline with Circuit Breaker
```cpp
class DataPipeline {
    unified_thread_system system_{
        config{}.set_name("Pipeline")
               .enable_circuit_breaker(true)
    };

public:
    void process_stream(DataStream& stream) {
        auto futures = system_.submit_batch(
            stream.begin(), stream.end(),
            [this](const Data& d) { return transform(d); }
        );

        for (auto& f : futures) {
            try {
                auto result = f.get();
                // Process result
            } catch (const std::exception& e) {
                if (system_.is_circuit_open()) {
                    // Circuit opened - stop processing
                    break;
                }
            }
        }
    }
};
```

### Real-Time Image Processing
```cpp
auto process_images(const std::vector<Image>& images) {
    return system.map_reduce(
        images.begin(), images.end(),
        [](const Image& img) { return apply_filters(img); },  // Map
        [](Results& a, Result b) { return merge(a, b); },     // Reduce
        Results{}                                              // Initial
    );
}
```

[📖 View More Examples](docs/EXAMPLES.md)

## 📊 Performance Benchmarks

### Task Throughput Comparison

```
Single Thread    |████                    |  150K tasks/sec
std::async       |██████                  |  280K tasks/sec
Basic ThreadPool |████████████            |  650K tasks/sec
THIS PROJECT     |████████████████████████| 1.16M tasks/sec 🚀
```

### Latency Distribution (microseconds)

| Percentile | Simple Task | Complex Task | With Logging |
|------------|------------|--------------|--------------|
| p50        | 0.8 μs     | 12 μs        | 15 μs        |
| p90        | 1.2 μs     | 18 μs        | 22 μs        |
| p95        | 1.5 μs     | 24 μs        | 28 μs        |
| p99        | 2.8 μs     | 45 μs        | 52 μs        |
| p99.9      | 5.2 μs     | 89 μs        | 95 μs        |

### Scalability (8-core System)

```
Cores  Throughput   Speedup   Efficiency
1      145K/s       1.0x      100%
2      285K/s       1.97x     98%
4      560K/s       3.86x     96%
8      1,160K/s     8.0x      100%
16     1,180K/s     8.14x     51% (HT)
```

## 🏗️ Architecture

```mermaid
graph TB
    subgraph "Your Application"
        APP[Application Code]
    end

    subgraph "Unified Thread System"
        API[Simple API Layer]
        SCHED[Smart Scheduler]

        subgraph "Core Components"
            TP[Thread Pool<br/>1.16M+ ops/sec]
            LOG[Logger System<br/>Async, Structured]
            MON[Monitor System<br/>Real-time Metrics]
        end
    end

    subgraph "Enhanced Features"
        PRIO[Priority Queue]
        CB[Circuit Breaker]
        EVENT[Event Bus]
        CANCEL[Cancellation]
    end

    APP --> API
    API --> SCHED
    SCHED --> TP
    SCHED --> PRIO
    TP --> LOG
    TP --> MON
    PRIO --> CB
    CB --> EVENT
```

## 🛠️ Configuration

### Simple Configuration
```cpp
config cfg;
cfg.name = "MyApp";
cfg.thread_count = 8;  // Or 0 for auto-detect
cfg.enable_file_logging = true;

unified_thread_system system(cfg);
```

### Advanced Configuration
```cpp
config cfg;
cfg.set_name("DataProcessor")
   .set_worker_count(16)
   .enable_work_stealing(true)
   .enable_circuit_breaker(true)
   .set_circuit_breaker_threshold(5)
   .set_max_queue_size(100000);

unified_thread_system system(cfg);
```

[📋 View All Configuration Options](docs/API.md#configuration)

## 🌐 Platform Support

### Supported Platforms & Compilers

| Platform | Compiler | Version | Status | CI/CD | Notes |
|----------|----------|---------|--------|-------|-------|
| **Ubuntu 22.04+** | GCC | 11+ | ✅ Fully Tested | [![Ubuntu-GCC](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-gcc.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-gcc.yaml) | Primary development platform |
| **Ubuntu 22.04+** | Clang | 14+ | ✅ Fully Tested | [![Ubuntu-Clang](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-clang.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-ubuntu-clang.yaml) | Full C++20 support |
| **Windows 10+** | MSVC | 2019+ | ✅ Fully Tested | [![Windows-VS](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-vs.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-vs.yaml) | Visual Studio 2019/2022 |
| **Windows 10+** | MSYS2 (GCC) | 11+ | ✅ Fully Tested | [![Windows-MSYS2](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-msys2.yaml/badge.svg)](https://github.com/kcenon/integrated_thread_system/actions/workflows/build-windows-msys2.yaml) | MinGW-w64 toolchain |
| **macOS 13+** | Apple Clang | 14+ | ✅ Tested | - | Intel and Apple Silicon (M1/M2/M3) |
| **Linux (Other)** | GCC/Clang | 11+/14+ | ✅ Expected | - | Should work on most modern distros |

### Architecture Support

| Architecture | Status | Performance Notes |
|--------------|--------|-------------------|
| **x86_64** | ✅ Fully Optimized | Primary target, all optimizations enabled |
| **ARM64 (AArch64)** | ✅ Fully Optimized | Apple Silicon, AWS Graviton tested |
| **x86 (32-bit)** | ⚠️ Not Tested | May work but not officially supported |
| **ARM (32-bit)** | ⚠️ Not Tested | May work but not officially supported |

### C++ Standard Requirements

- **Minimum**: C++20 (required for `std::jthread`, concepts, ranges)
- **Recommended**: C++20 with full library support
- **Future**: C++23 compatibility planned (`std::expected` migration)

### Build System Support

| Build System | Status | Notes |
|--------------|--------|-------|
| **CMake** | ✅ 3.16+ | Primary build system, full feature support |
| **vcpkg** | ✅ Supported | Package management via vcpkg.json |
| **Conan** | ⚠️ Experimental | Community-maintained recipes |
| **Bazel** | ❌ Not Supported | Contributions welcome |

### Feature Availability by Platform

| Feature | Linux | Windows | macOS | Notes |
|---------|-------|---------|-------|-------|
| Thread Pools | ✅ | ✅ | ✅ | Full support on all platforms |
| Priority Scheduling | ✅ | ✅ | ✅ | Full support on all platforms |
| Lock-free Queues | ✅ | ✅ | ✅ | Full support on all platforms |
| Async Logging | ✅ | ✅ | ✅ | Full support on all platforms |
| File Logging | ✅ | ✅ | ✅ | Platform-specific path handling |
| Network Logging | ✅ | ✅ | ✅ | TCP/UDP support on all platforms |
| Prometheus Metrics | ✅ | ✅ | ✅ | Full support on all platforms |
| Health Monitoring | ✅ | ✅ | ✅ | Full support on all platforms |
| System Metrics | ✅ | ✅ | ✅ | Platform-specific implementations |

### Known Platform-Specific Issues

**Windows:**
- Network logging requires Winsock initialization
- File paths use backslashes by default (normalized internally)
- Some POSIX-specific examples may need adaptation

**macOS:**
- System metrics use different APIs than Linux
- Some sanitizer tests disabled (platform limitations)

**Linux:**
- Full POSIX compliance
- All features fully supported
- Best performance characteristics

> 📖 For detailed platform-specific build instructions, see [BUILDING.md](docs/BUILDING.md)

## 📈 Monitoring & Observability

### Real-Time Metrics
```cpp
auto metrics = system.get_metrics();
std::cout << "Throughput: " << metrics.tasks_per_second << " tasks/sec\n";
std::cout << "P99 Latency: " << metrics.p99_latency.count() << " ns\n";
std::cout << "Queue Usage: " << metrics.queue_utilization_percent << "%\n";
```

### Health Monitoring
```cpp
auto health = system.get_health();
if (health.overall_health == health_level::degraded) {
    for (const auto& issue : health.issues) {
        LOG_WARNING("Health issue: {}", issue);
    }
}
```

### Export Formats
```cpp
// Prometheus format
std::string prometheus = system.export_metrics_prometheus();

// JSON format
std::string json = system.export_metrics_json();
```

## 🔧 Build Options

### Using CMake
```cmake
find_package(integrated_thread_system REQUIRED)
target_link_libraries(myapp PRIVATE
    integrated_thread_system::integrated_thread_system)
```

### Using build.sh
```bash
# Standard build
./build.sh

# With specific compiler
./build.sh --compiler clang++

# Debug build
./build.sh --debug

# With tests and benchmarks
./build.sh --with-tests --with-benchmarks
```

### Manual Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 📚 Documentation

- **[📖 Getting Started Guide](docs/getting_started/)** - Quick tutorials and setup
- **[🔧 API Reference](docs/API.md)** - Complete API documentation
- **[💡 Examples](docs/EXAMPLES.md)** - 16+ real-world examples
- **[🏗️ Architecture Guide](docs/architecture/)** - System design details
- **[📊 Performance Tuning](docs/guides/performance.md)** - Optimization guide
- **[🔄 Migration Guide](CHANGELOG.md#migration-guide)** - Upgrading from older versions

## 🤝 Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup
```bash
git clone https://github.com/kcenon/integrated_thread_system.git
cd integrated_thread_system
./build.sh --debug --with-tests
./run_tests.sh
```

## 📊 Project Statistics

<table>
<tr>
<td>

**Codebase**
- 📝 ~5,000 lines of core code
- 🧪 95%+ test coverage
- 📚 Comprehensive documentation
- 🔍 Zero known bugs

</td>
<td>

**Performance**
- ⚡ 1.16M+ tasks/second
- 🎯 < 1μs p50 latency
- 📈 Linear scalability
- 💾 < 100MB memory

</td>
<td>

**Community**
- ⭐ Stars welcome!
- 🐛 Issues tracked
- 🔀 PRs reviewed
- 💬 Active support

</td>
</tr>
</table>

## 🏆 Used By

This project integrates and enhances:
- **[thread_system](https://github.com/kcenon/thread_system)** - High-performance threading
- **[logger_system](https://github.com/kcenon/logger_system)** - Async structured logging
- **[monitoring_system](https://github.com/kcenon/monitoring_system)** - Observability platform

## 🏗️ Production Quality & Architecture

integrated_thread_system delivers production-ready unified threading capabilities with comprehensive quality assurance across all integrated components.

### Build & Testing Infrastructure

**Multi-Component CI/CD Pipeline**
- Automated builds across all integrated systems (thread, logger, monitoring)
- Cross-platform testing: Ubuntu (GCC/Clang), Windows (MSVC/MSYS2), macOS (Apple Clang)
- Unified test coverage tracking across components (~85%+ combined coverage)
- Sanitizer validation: ThreadSanitizer, AddressSanitizer, UBSanitizer
- Integration smoke tests validating component interactions

**Performance Validation**
- Integrated system throughput: 1.16M+ tasks/second (full logging + monitoring)
- Component isolation: Each system validated individually before integration
- Regression detection with automated baseline comparison
- End-to-end latency profiling across all layers

### Integration Quality Assurance

**Component Integration Validation**
- Thread system ↔ Logger system: Async logging integration verified
- Thread system ↔ Monitoring system: Real-time metrics collection validated
- Logger ↔ Monitoring: Unified observability pipeline tested
- Cross-component error handling via Result<T> pattern verified

**Thread Safety Across Components**
- All component interactions validated with ThreadSanitizer
- Zero data races in integrated scenarios
- Concurrent logger/monitor access from thread pools verified
- Lock hierarchy maintained across system boundaries

### Resource Management (RAII - Grade A)

**Unified Resource Lifecycle**
- All components use consistent RAII patterns
- Smart pointer usage: 100% across all integrated systems
- Automatic cleanup on shutdown: thread pools → logger → monitoring
- AddressSanitizer validation: Zero leaks in integrated scenarios

**Validation Results**
```bash
# Integrated system: AddressSanitizer clean
- Thread pools: 0 leaks
- Logger system: 0 leaks
- Monitoring system: 0 leaks
- Integration layer: 0 leaks
Total: 0 memory leaks detected
```

### Error Handling (Production Ready - 95% Complete)

**Unified Result<T> Pattern**
All components use consistent error handling via common_system:
- Thread operations: Result<void> for all async operations
- Logger operations: Result<void> for write/flush operations
- Monitoring operations: Result<void> for metric collection
- Integration layer: Result<T> for component initialization

**Error Code Coordination**
Centralized error code registry prevents conflicts:
- Common system: -1 to -99
- Thread system: -100 to -199
- Logger system: -200 to -299
- Monitoring system: -300 to -399
- Integration layer: -400 to -499 (reserved)

**Example: Integrated Error Handling**
```cpp
// All components return Result<T> for explicit error handling
auto system_result = unified_thread_system::create(config);
if (!system_result) {
    const auto& err = system_result.get_error();
    std::cerr << "System initialization failed: " << err.message()
              << " (code: " << static_cast<int>(err.code())
              << ", source: " << err.source() << ")\n";
    // Error codes indicate which component failed:
    // -100 to -199: Thread system issue
    // -200 to -299: Logger system issue
    // -300 to -399: Monitoring system issue
    return -1;
}

auto system = std::move(system_result.value());

// Submit task with comprehensive error handling
auto task_result = system.submit([]() {
    return process_data();
});

if (!task_result) {
    // Handle submission error with full context
    log_error(task_result.get_error());
}
```

### Architecture Improvement Phases

**Phase Status Overview** (as of 2025-10-23):

| Phase | Status | Completion | Key Achievements |
|-------|--------|------------|------------------|
| **Phase 0**: Foundation | ✅ Complete | 100% | Unified build system, baseline integration metrics |
| **Phase 1**: Thread Safety | ✅ Complete | 100% | Cross-component ThreadSanitizer validation |
| **Phase 2**: Resource Management | ✅ Complete | 100% | Grade A RAII across all components |
| **Phase 3**: Error Handling | ✅ Complete | 95% | Unified Result<T> pattern, coordinated error codes |
| **Phase 4**: API Refinement | 🔄 In Progress | 60% | Simplified configuration, builder patterns |
| **Phase 5**: Performance Tuning | ⏳ Planned | 0% | Cross-component optimization |
| **Phase 6**: Documentation | 🔄 In Progress | 70% | Comprehensive guides and examples |

**Phase 3 - Error Handling Unification: Integration Layer**

integrated_thread_system implements the **Integration Layer** pattern where components communicate via Result<T>:

**Implementation Status**: 95% Complete
- ✅ All component interactions return Result<T> for explicit error handling
- ✅ Unified error code coordination prevents conflicts across systems
- ✅ Comprehensive error context with source component identification
- ✅ Error propagation tested across component boundaries
- ✅ Integration tests validate error handling in failure scenarios

**Error Code Organization**:
- Integration layer: -400 to -499 (reserved for future use)
- Component initialization: -400 to -409
- Configuration validation: -410 to -419
- Runtime coordination: -420 to -429

**Integration Pattern**:
```cpp
// Component initialization with Result<T>
auto thread_pool = thread_system::create_pool(config);
if (!thread_pool) {
    return propagate_error<unified_thread_system>(thread_pool.get_error());
}

auto logger = logger_system::create_logger(log_config);
if (!logger) {
    return propagate_error<unified_thread_system>(logger.get_error());
}

auto monitor = monitoring_system::create_monitor(monitor_config);
if (!monitor) {
    return propagate_error<unified_thread_system>(monitor.get_error());
}

// Successful integration
return ok(unified_thread_system{
    std::move(thread_pool.value()),
    std::move(logger.value()),
    std::move(monitor.value())
});
```

**Benefits**:
- Explicit error handling at integration boundaries
- Clear identification of which component failed
- Type-safe error propagation across system boundaries
- Comprehensive error context for debugging

**Remaining Work** (5%):
- Optional: Additional integration error scenario tests
- Optional: Enhanced error recovery strategies
- Optional: Error aggregation for batch operations

---

For detailed architecture improvements and ecosystem-wide status, see individual component repositories:
- [thread_system architecture](https://github.com/kcenon/thread_system#production-quality--architecture)
- [logger_system architecture](https://github.com/kcenon/logger_system#production-quality--architecture)
- [monitoring_system architecture](https://github.com/kcenon/monitoring_system#production-quality--architecture)

## 📄 License

BSD 3-Clause License - see [LICENSE](LICENSE) for details.

## 📬 Contact & Support

- **Author**: kcenon ([@kcenon](https://github.com/kcenon))
- **Email**: kcenon@gmail.com
- **Issues**: [GitHub Issues](https://github.com/kcenon/integrated_thread_system/issues)
- **Discussions**: [GitHub Discussions](https://github.com/kcenon/integrated_thread_system/discussions)

---

<div align="center">

**If this project helps you, please consider giving it a ⭐!**

Made with ❤️ by the Open Source Community

</div>