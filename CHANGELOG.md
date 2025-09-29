# Changelog

All notable changes to the Integrated Thread System will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
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
- Missing `log_level::fatal` case in switch statements
- Template instantiation errors in implementation files
- Include path issues across example and test files
- Unused parameter warnings with C++17 fold expressions

### Changed
- Unified constructor interface using default parameters
- Improved header organization with config struct moved outside class
- Enhanced performance metrics structure with additional fields
- Updated health status structure with circuit breaker information

### Documentation
- Complete API reference with all public methods
- Comprehensive examples guide with 16 real-world scenarios
- Updated README with clear feature comparison
- Performance optimization guidelines
- Best practices and patterns documentation

## [1.0.0] - 2024-01-15

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

## [0.9.0] - 2024-01-01 (Pre-release)

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