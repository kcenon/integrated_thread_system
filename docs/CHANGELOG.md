# Integrated Thread System Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- **C++20 Concepts Integration** (Issue #71)
  - Updated common_system dependency from v1.0.0 to v2.0.0
  - Added `std::invocable` constraints to all template functions for better compile-time validation
  - Added local `VoidCallable` concept for void-returning callables
  - Service registry methods use `std::is_base_of_v` and `std::is_polymorphic_v` constraints
  - Benefits: Clearer error messages, better IDE support, self-documenting API

### Added
- Documentation structure standardization
- Korean documentation support

---

## [1.0.0] - 2025-11-12

### Added
- **Unified Integration**: Combined thread_system, logger_system, and monitoring_system
  - Zero-configuration setup
  - Enterprise-grade threading framework
  - Integrated observability
- **Configuration System**: Centralized configuration management
  - Adaptive monitoring support
  - Scheduler configuration
  - Enhanced reliability features
- **Component Integration**:
  - thread_system v1.0.0: Lock-free thread pools, priority scheduling
  - logger_system v1.0.0: Async logging (4.34M+ logs/sec)
  - monitoring_system v2.0.0: Adaptive monitoring, health checks

### Dependencies
- common_system v1.0.0: Result<T>, standalone event bus
- thread_system v1.0.0: Scheduler, service registry, crash handler
- logger_system v1.0.0: Backend architecture, formatters, security
- monitoring_system v2.0.0: Adaptive monitoring, health checks, reliability

---

## [0.9.0] - 2025-10-15

### Added
- Initial integration of core subsystems
- Basic configuration system
- Integration test framework

### Changed
- Migrated to CMake FetchContent for dependency management

---

*For detailed changes in individual components, see:*
- [thread_system CHANGELOG](https://github.com/kcenon/thread_system/blob/main/CHANGELOG.md)
- [logger_system CHANGELOG](https://github.com/kcenon/logger_system/blob/main/CHANGELOG.md)
- [monitoring_system CHANGELOG](https://github.com/kcenon/monitoring_system/blob/main/CHANGELOG.md)
