# Changelog

All notable changes to the Integrated Thread System will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added - External System Integrations
- **thread_system Integration** (PR #8)
  - Full integration with `kcenon::thread::thread_pool` for task execution
  - Support for both `thread_pool` and `typed_thread_pool` (standard and priority-based)
  - Cancellation token support using `kcenon::thread::cancellation_token`
  - Type-erased cancellation tokens via `std::shared_ptr<void>` for flexible implementation
  - Manual worker thread creation and management
  - Job queue integration with worker threads

- **logger_system Integration** (PR #9)
  - Async logging integration using `kcenon::logger::logger` with 8KB buffer
  - `console_writer` and `file_writer` integration for flexible output
  - Log level conversion supporting all levels (trace through fatal)
  - Thread-safe logging across all subsystems
  - Automatic flush on shutdown

- **monitoring_system Integration** (PR #10)
  - `performance_profiler` integration for statistical analysis
  - `system_monitor` integration for real-time CPU and memory tracking
  - Enhanced metrics with mean duration, p95 latency, and call counts
  - Health monitoring with configurable thresholds (90% CPU/memory)
  - Maximum samples per metric configuration (default: 10,000)
  - System resource monitoring (CPU usage, memory usage)

### Enhanced Features
- Enhanced version (`integrated_thread_system_enhanced`) with advanced features
  - Priority-based task scheduling (critical, normal, background)
  - Cancellation tokens for safe task cancellation
  - Scheduled and recurring task execution
  - Map-reduce pattern for parallel data processing
  - Circuit breaker pattern for failure isolation
  - Event publish-subscribe system
  - Plugin system for dynamic feature extension
  - Extended performance metrics (p95, p99 latency, throughput)
  - Health monitoring with issue detection

### Fixed
- Constructor ambiguity between default constructor and constructor with default parameter
- Missing `log_level::fatal` case in switch statements (maps to `critical` in thread_system)
- Template instantiation errors in implementation files
- Include path issues across example and test files
- Unused parameter warnings with C++17 fold expressions
- PIMPL pattern template compatibility with delegation methods
- Result/Error handling API differences (is_err() vs has_error())
- Thread pool worker creation (workers must be manually added before starting)

### Changed
- Unified constructor interface using default parameters
- Improved header organization with config struct moved outside class
- Enhanced performance metrics structure with additional fields
- Updated health status structure with circuit breaker information
- CMake configuration to detect and link external system libraries
- Adapter implementations to use external systems when available
- Conditional compilation with `EXTERNAL_SYSTEMS_AVAILABLE` macro

### Implementation Details
- **Dual Implementation Pattern**: All adapters maintain both external system integration and fallback implementations
- **Type Erasure**: Cancellation tokens use `std::shared_ptr<void>` for compatibility across implementations
- **Promise-based Exception Handling**: Uses `std::promise` instead of `std::packaged_task` for comprehensive exception propagation
- **Statistical Profiling**: Metrics include not just raw values but statistical analysis (mean, p95, call counts)
- **Threshold-based Health Monitoring**: Configurable thresholds for detecting degraded system states

### Documentation
- Updated README with detailed external system integration information
- Complete API reference with cancellation token methods
- Comprehensive examples guide with advanced cancellation patterns
- Updated CHANGELOG with integration milestone details
- Performance optimization guidelines
- Best practices and patterns documentation

## [1.0.0]

### Added
- Initial release of Integrated Thread System
- Zero-configuration thread pool with automatic worker scaling
- Integrated logging system with file and console output
- Performance monitoring with real-time metrics
- Basic task submission and batch processing
- Thread pool health monitoring
- Graceful shutdown handling
- Work stealing between worker threads
- Comprehensive test suite
- Cross-platform support (Linux, macOS, Windows)
- CMake build system with vcpkg integration

### Features
- **Thread Pool Management**
  - Automatic CPU core detection
  - Dynamic worker scaling
  - Work stealing algorithm
  - Queue size management

- **Logging Integration**
  - Multiple log levels (trace, debug, info, warning, error, critical)
  - File and console output
  - Structured logging support
  - Configurable log directory

- **Performance Monitoring**
  - Task submission/completion metrics
  - Average latency tracking
  - Queue utilization monitoring
  - Worker thread statistics

### Dependencies
- C++20 compiler support
- fmt library for formatting
- nlohmann_json for JSON support
- Google Test for unit testing (optional)

## [0.9.0] (Pre-release)

### Added
- Integration of thread_system, logger_system, and monitoring_system
- Fallback mechanisms for missing external systems
- Basic unified API design
- Initial test framework

### Known Issues
- API not fully stabilized
- Documentation incomplete
- Some edge cases in error handling

## Comparison

### Standard vs Enhanced Version

| Feature | Standard | Enhanced |
|---------|----------|----------|
| Zero-configuration | ✅ | ✅ |
| Thread pool | ✅ | ✅ |
| Logging | ✅ | ✅ |
| Basic monitoring | ✅ | ✅ |
| Priority scheduling | ❌ | ✅ |
| Cancellation | ❌ | ✅ |
| Scheduled tasks | ❌ | ✅ |
| Recurring tasks | ❌ | ✅ |
| Map-reduce | ❌ | ✅ |
| Circuit breaker | ❌ | ✅ |
| Event system | ❌ | ✅ |
| Plugin support | ❌ | ✅ |
| Extended metrics | ❌ | ✅ |

## Migration Guide

### From v0.9.0 to v1.0.0

1. **Constructor changes**:
   ```cpp
   // Old (v0.9.0)
   unified_thread_system system;
   unified_thread_system system(config);

   // New (v1.0.0) - Single constructor with default parameter
   unified_thread_system system;  // Uses default config
   unified_thread_system system(config);  // Uses custom config
   ```

2. **Enhanced features (optional)**:
   ```cpp
   // To use enhanced features, link with integrated_thread_system_enhanced
   // and use the new API methods:
   system.submit_critical(...);
   system.submit_cancellable(...);
   system.map_reduce(...);
   ```

3. **Monitoring improvements**:
   ```cpp
   // New metrics available
   auto metrics = system.get_metrics();
   std::cout << "P95 latency: " << metrics.p95_latency.count() << "ns\n";
   std::cout << "Tasks/sec: " << metrics.tasks_per_second << "\n";
   ```

## Performance Improvements

### Benchmark Results (v1.0.0 Enhanced)

- **Task throughput**: 1.16M+ tasks/second
- **Average latency**: < 20μs for simple tasks
- **Parallel speedup**: 7.62x on 8-core system
- **Memory usage**: < 100MB for 10,000 queued tasks
- **Circuit breaker response**: < 1ms failure detection

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on contributing to this project.

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.