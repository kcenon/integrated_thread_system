# Integrated Thread System

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)

## Overview

The **Integrated Thread System** is a comprehensive C++20 framework that unifies three high-performance systems:

- **[thread_system](https://github.com/kcenon/thread_system)**: Core threading framework with adaptive queues (1.16M+ jobs/sec)
- **[logger_system](https://github.com/kcenon/logger_system)**: Asynchronous structured logging system
- **[monitoring_system](https://github.com/kcenon/monitoring_system)**: Real-time metrics collection and web dashboard

This integration provides enterprise-grade observability, performance monitoring, and operational excellence while maintaining the individual strengths of each system.

## ✨ Key Features

### 🚀 **Unified Performance**
- **High Throughput**: Maintains 1.16M+ jobs/second performance
- **Adaptive Queues**: Automatic optimization based on workload patterns
- **Low Latency**: <5% integration overhead
- **Scalable Architecture**: Linear scaling with hardware threads

### 📊 **Complete Observability**
- **Structured Logging**: Automatic context-aware logging for all operations
- **Real-time Metrics**: Live performance monitoring and dashboards
- **Intelligent Alerting**: Rule-based notifications with multi-channel support
- **Web Dashboard**: Interactive real-time visualization

### 🛡️ **Enterprise-Grade Reliability**
- **Graceful Degradation**: System continues operating even if components fail
- **Health Monitoring**: Automatic health checks and recovery mechanisms
- **Configuration Management**: JSON-based unified configuration system
- **Error Handling**: Comprehensive Result<T> pattern for robust error management

### 🔧 **Developer Experience**
- **Simple Integration**: Single configuration file controls all systems
- **Fluent API**: Intuitive, self-documenting interfaces
- **Application Framework**: High-level framework for common patterns
- **Comprehensive Examples**: Production-ready code examples

## 🚀 Quick Start

### Prerequisites

- C++20 capable compiler (GCC 11+, Clang 14+, MSVC 2019+)
- CMake 3.16 or later
- vcpkg (optional, for dependency management)

### Installation

```bash
# Clone with submodules
git clone --recursive https://github.com/kcenon/integrated_thread_system.git
cd integrated_thread_system

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Run tests (optional)
ctest --verbose
```

### Basic Usage

```cpp
#include <kcenon/integrated/framework/application.h>
#include <iostream>

using namespace kcenon::integrated::framework;

int main() {
    // Create application with default configuration
    application_framework::application_config config;
    config.name = "My Application";
    config.config_file_path = "config/production.json";

    application_framework app(config);

    // Initialize (automatically starts all systems)
    auto result = app.initialize();
    if (!result) {
        std::cerr << "Failed to initialize: " << result.get_error().message();
        return 1;
    }

    // Get integrated components
    auto thread_pool = app.get_thread_pool();
    auto logger = app.get_logger();

    // Submit instrumented tasks with automatic logging and monitoring
    auto future = thread_pool->submit_task("data_processing", []() {
        // Your task logic here
        return 42;
    });

    if (future) {
        auto result = future.value().get();
        logger->info("Task completed with result: {}", result);
    }

    // Performance metrics are automatically collected
    auto metrics = thread_pool->get_performance_metrics();
    logger->info("Processed {} tasks", metrics.tasks_completed);

    return 0;
}
```

### Configuration Example

```json
{
  "integrated_thread_system": {
    "version": "1.0.0",
    "profile": "production",
    "systems": {
      "thread_system": {
        "enabled": true,
        "thread_pools": {
          "default": {
            "workers": "auto",
            "queue_type": "adaptive",
            "batch_processing": true
          }
        }
      },
      "logger_system": {
        "enabled": true,
        "level": "info",
        "structured": true,
        "writers": ["console", "file"]
      },
      "monitoring_system": {
        "enabled": true,
        "web_dashboard": {
          "port": 8080,
          "enabled": true
        }
      }
    }
  }
}
```

## 📊 Performance Benchmarks

*Benchmarked on Intel i7-12700K @ 5.0GHz, 32GB DDR5, Ubuntu 22.04, GCC 11.3*

| Metric | Individual Systems | Integrated System | Impact |
|--------|-------------------|-------------------|---------|
| **Throughput** | 1.16M jobs/sec | 1.14M jobs/sec | -1.7% |
| **Latency** | 77 ns/job | 81 ns/job | +5.2% |
| **Memory Usage** | 15MB | 23MB | +8MB |
| **CPU Overhead** | 2.1% | 2.3% | +0.2% |

### Integration Benefits vs. Overhead

✅ **Benefits**:
- Complete observability out of the box
- Unified configuration and management
- Automatic performance monitoring
- Structured logging for all operations
- Real-time web dashboard
- Intelligent alerting system

⚠️ **Overhead**: Minimal impact (<2% performance decrease) for comprehensive observability

## 🏗️ Architecture

### System Integration

```
┌─────────────────────────────────────────────┐
│            Web Dashboard                     │
│   Real-time Metrics + Logs + Alerts        │
└─────────────────────────────────────────────┘
                        │
                   WebSocket/HTTP
                        │
┌─────────────────────────────────────────────┐
│        Integrated Thread System             │
│  ┌─────────────┐ ┌─────────────────────┐    │
│  │Integration  │ │ Unified Config &    │    │
│  │Manager      │ │ Service Registry    │    │
│  └─────────────┘ └─────────────────────┘    │
└─────────────────────────────────────────────┘
                        │
    ┌───────────────────┼───────────────────┐
    │                   │                   │
    ▼                   ▼                   ▼
┌────────────┐  ┌───────────────┐  ┌─────────────┐
│thread_     │  │logger_        │  │monitoring_  │
│system      │  │system         │  │system       │
│            │  │               │  │             │
│• Adaptive  │  │• Async Logs   │  │• Real-time  │
│  Queues    │  │• Structured   │  │  Alerts     │
│• High      │  │• Multi-target │  │• Web UI     │
│  Performance│  │  Output       │  │• Metrics    │
└────────────┘  └───────────────┘  └─────────────┘
```

### Key Components

- **Integration Manager**: Central orchestrator managing system lifecycle
- **Service Registry**: Dependency injection container for loose coupling
- **Facade Pattern**: Simplified interfaces for complex subsystems
- **Unified Thread Pool**: Thread pool with integrated logging and monitoring
- **Application Framework**: High-level framework for common application patterns

## 📚 Documentation

### Core Documentation
- [**Design Document**](docs/DESIGN_DOCUMENT.md) - Comprehensive system design
- [**Architecture Guide**](docs/ARCHITECTURE.md) - Detailed architecture and patterns
- [**Integration Guide**](docs/INTEGRATION_GUIDE.md) - Step-by-step integration instructions
- [**API Reference**](docs/API_REFERENCE.md) - Complete API documentation

### Examples
- [**Basic Example**](examples/basic_example.cpp) - Simple integration example
- [**Complete Application**](examples/complete_application.cpp) - Full-featured example
- [**Performance Testing**](examples/performance_benchmark.cpp) - Performance validation
- [**Custom Configuration**](examples/custom_configuration.cpp) - Advanced configuration

### Configuration
- [**Production Config**](config/production.json) - Production settings
- [**Development Config**](config/development.json) - Development settings
- [**High Performance Config**](config/high_performance.json) - Maximum throughput
- [**Debug Config**](config/debug.json) - Debugging and troubleshooting

## 🔧 Building from Source

### CMake Options

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_BENCHMARKS=ON \
  -DENABLE_WEB_DASHBOARD=ON
```

### Available Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | ON | Build unit and integration tests |
| `BUILD_EXAMPLES` | ON | Build example applications |
| `BUILD_BENCHMARKS` | OFF | Build performance benchmarks |
| `ENABLE_WEB_DASHBOARD` | ON | Enable web dashboard features |
| `USE_SYSTEM_DEPENDENCIES` | OFF | Use system-installed dependencies |

## 🧪 Testing

```bash
# Run all tests
ctest --verbose

# Run specific test categories
./build/tests/unit_tests
./build/tests/integration_tests
./build/tests/e2e_tests

# Run performance benchmarks
./build/benchmarks/performance_benchmark

# Run long-running stability tests
./build/tests/stability_tests --duration=3600
```

### Test Coverage

- **Unit Tests**: Individual component testing with mocks
- **Integration Tests**: Cross-system integration validation
- **E2E Tests**: Complete workflow and scenario testing
- **Performance Tests**: Throughput and latency validation
- **Stability Tests**: Long-running reliability validation

## 🚀 Production Deployment

### Configuration Management

The system supports environment-based configuration:

```bash
# Set environment-specific configuration
export INTEGRATED_THREAD_SYSTEM_ENV=production
export INTEGRATED_THREAD_SYSTEM_CONFIG=/etc/integrated_thread_system/config.json

# Run application
./my_application
```

### Docker Support

```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config

# Copy application
COPY . /app
WORKDIR /app

# Build and install
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . && \
    cmake --install .

# Run
CMD ["./build/my_application"]
```

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](docs/CONTRIBUTING.md) for details.

### Development Setup

```bash
# Clone with development setup
git clone --recursive https://github.com/kcenon/integrated_thread_system.git
cd integrated_thread_system

# Install development dependencies
vcpkg install gtest benchmark nlohmann-json

# Build with all options
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build .

# Run tests
ctest
```

## 📄 License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [**thread_system**](https://github.com/kcenon/thread_system) - High-performance threading framework
- [**logger_system**](https://github.com/kcenon/logger_system) - Asynchronous logging system
- [**monitoring_system**](https://github.com/kcenon/monitoring_system) - Real-time monitoring platform
- OpenTelemetry community for standardization efforts
- Contributors and users for valuable feedback

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/kcenon/integrated_thread_system/issues)
- **Discussions**: [GitHub Discussions](https://github.com/kcenon/integrated_thread_system/discussions)
- **Email**: kcenon@naver.com

---

<p align="center">
  <strong>integrated_thread_system</strong> - Enterprise-grade Unified Threading Framework<br>
  Made with ❤️ by the integrated_thread_system team
</p>