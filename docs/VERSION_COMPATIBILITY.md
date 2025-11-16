# Version Compatibility Matrix

This document provides a comprehensive version compatibility matrix for `integrated_thread_system` and its subsystem dependencies.

## Current Version Requirements

**integrated_thread_system v2.0.0** requires the following dependency versions:

| Component | Required Version | Status | Repository |
|-----------|-----------------|--------|------------|
| **common_system** | v1.0.0+ | ✅ Stable | [kcenon/common_system](https://github.com/kcenon/common_system) |
| **thread_system** | v1.0.0+ | ✅ Stable | [kcenon/thread_system](https://github.com/kcenon/thread_system) |
| **logger_system** | v1.0.0+ | ✅ Stable | [kcenon/logger_system](https://github.com/kcenon/logger_system) |
| **monitoring_system** | v2.0.0+ | ✅ Stable | [kcenon/monitoring_system](https://github.com/kcenon/monitoring_system) |

> **Note**: The "+" indicates that the system is compatible with the specified version and all patch versions within the same major.minor version (e.g., v1.0.0, v1.0.1, v1.0.2, etc.).

---

## Feature Compatibility by Subsystem Version

### thread_system

| Version | Features | integrated_thread_system Support |
|---------|----------|----------------------------------|
| **v1.0.0** | Thread pool, typed thread pool, job queue, scheduler interface, service registry, crash handler, hazard pointer support | ✅ Fully Supported |
| v0.9.x | Thread pool, job queue (no scheduler, no service registry) | ⚠️ Legacy (not recommended) |

**Key Features in v1.0.0**:
- Scheduler interface for advanced task scheduling
- Service registry with dependency injection container
- Signal-safe crash handler for production robustness
- Hazard pointer support for lock-free queue (experimental)
- Bounded queue option for memory-constrained environments

### logger_system

| Version | Features | integrated_thread_system Support |
|---------|----------|----------------------------------|
| **v1.0.0** | Backend architecture (console/file), formatters, security features, async logging | ✅ Fully Supported |
| v0.9.x | Basic console/file logging (no backend abstraction) | ⚠️ Legacy (not recommended) |

**Key Features in v1.0.0**:
- Backend architecture for pluggable log outputs
- Advanced formatters (JSON, custom formats)
- Security features (log sanitization, PII redaction)
- High-performance async logging (4.34M+ logs/sec)

### monitoring_system

| Version | Features | integrated_thread_system Support |
|---------|----------|----------------------------------|
| **v2.0.0** | Adaptive monitoring, health checks, reliability features (error boundary, fault tolerance, retry policy) | ✅ Fully Supported |
| v1.0.x | Performance profiler, system monitor (no adaptive features) | ⚠️ Partial (basic metrics only) |

**Key Features in v2.0.0**:
- **Adaptive Monitoring**: Automatically adjusts sampling rate based on system load
  - Low load (< 30%): Sample every 5 seconds
  - High load (> 70%): Sample every 100ms
- **Health Monitoring**: Periodic health checks with degradation detection
- **Specialized Collectors**:
  - Thread pool metrics collector
  - Logger metrics collector
  - System resource collector (CPU/memory)
  - Plugin metrics collector
- **Reliability Features**:
  - Error boundary for fault isolation
  - Fault tolerance manager
  - Automatic retry policy with exponential backoff

### common_system

| Version | Features | integrated_thread_system Support |
|---------|----------|----------------------------------|
| **v1.0.0** | Result<T> pattern, standalone event bus, error codes | ✅ Fully Supported |
| v0.9.x | Result<T> pattern only (no standalone event bus) | ⚠️ Legacy (not recommended) |

**Key Features in v1.0.0**:
- Standalone event bus (decoupled from messaging_system)
- Enhanced Result<T> pattern with error chaining
- Standardized error codes across all systems

---

## Version History and Upgrade Paths

### integrated_thread_system Releases

| Release | Date | thread_system | logger_system | monitoring_system | common_system | Notes |
|---------|------|---------------|---------------|-------------------|---------------|-------|
| **v2.0.0** | 2025-11-12 | v1.0.0 | v1.0.0 | v2.0.0 | v1.0.0 | Current release with adaptive monitoring |
| v1.0.0 | 2025-10-XX | v0.9.x | v0.9.x | v1.0.x | v0.9.x | Initial unified system release |

### Upgrade Paths

#### From v1.0.0 to v2.0.0

**Dependency Updates Required**:
1. **thread_system v0.9.x → v1.0.0**
   - **Breaking Changes**: Scheduler interface, service registry APIs
   - **Migration**: Enable new features via configuration flags (`enable_scheduler`, `enable_service_registry`)
   - **Backward Compatibility**: Legacy APIs still work with warnings

2. **logger_system v0.9.x → v1.0.0**
   - **Breaking Changes**: Backend architecture refactoring
   - **Migration**: Configuration changes for writer backends
   - **Backward Compatibility**: Console and file logging work as before

3. **monitoring_system v1.0.x → v2.0.0**
   - **Breaking Changes**: Adaptive monitoring API, health monitoring
   - **Migration**: Enable adaptive features via configuration
   - **Backward Compatibility**: Static sampling still available (default if adaptive disabled)

4. **common_system v0.9.x → v1.0.0**
   - **Breaking Changes**: Event bus namespace changes
   - **Migration**: Update event bus includes
   - **Backward Compatibility**: Result<T> API unchanged

**Recommended Upgrade Steps**:
1. Update all dependencies to latest versions
2. Enable new features incrementally via configuration
3. Test thoroughly in staging environment
4. Monitor performance and health metrics post-upgrade

---

## Version Pinning for Reproducible Builds

### CMake FetchContent Pinning

The `CMakeLists.txt` uses pinned Git tags for reproducible builds:

```cmake
# Pinned versions in CMakeLists.txt
set(REQUIRED_COMMON_SYSTEM_VERSION "1.0.0")
set(REQUIRED_THREAD_SYSTEM_VERSION "1.0.0")
set(REQUIRED_LOGGER_SYSTEM_VERSION "1.0.0")
set(REQUIRED_MONITORING_SYSTEM_VERSION "2.0.0")

# FetchContent uses these tags
FetchContent_Declare(
  thread_system
  GIT_REPOSITORY https://github.com/kcenon/thread_system.git
  GIT_TAG        v${REQUIRED_THREAD_SYSTEM_VERSION}
)
```

### vcpkg Version Constraints

If using vcpkg, specify versions in `vcpkg.json`:

```json
{
  "dependencies": [
    {
      "name": "thread-system",
      "version>=": "1.0.0"
    },
    {
      "name": "logger-system",
      "version>=": "1.0.0"
    },
    {
      "name": "monitoring-system",
      "version>=": "2.0.0"
    },
    {
      "name": "common-system",
      "version>=": "1.0.0"
    }
  ]
}
```

---

## Future Compatibility

### Planned Versions

| Component | Planned Version | Expected Features | ETA |
|-----------|----------------|-------------------|-----|
| thread_system | v2.0.0 | Plugin system, advanced scheduler (cron, deadline, dependency) | Q2 2026 |
| logger_system | v2.0.0 | Custom backends, structured logging, log streaming | Q2 2026 |
| monitoring_system | v3.0.0 | Distributed tracing, OpenTelemetry integration | Q3 2026 |
| integrated_thread_system | v2.1.0 | Full adaptive monitoring integration, enhanced testing | Q1 2026 |

### Backward Compatibility Guarantee

We follow [Semantic Versioning 2.0.0](https://semver.org/):

- **Major version (X.0.0)**: Breaking changes allowed
- **Minor version (x.Y.0)**: New features, backward compatible
- **Patch version (x.y.Z)**: Bug fixes, backward compatible

**Example**:
- `v2.0.0 → v2.0.1`: Drop-in replacement, no code changes needed
- `v2.0.0 → v2.1.0`: New features available, old code still works
- `v2.0.0 → v3.0.0`: Breaking changes, migration required

---

## Troubleshooting Version Issues

### Issue: "thread_system v2.0.0 not found"

**Symptom**: CMake fails with version not found error

**Cause**: thread_system v2.0.0 is not yet released (planned for Q2 2026)

**Solution**: Ensure `CMakeLists.txt` specifies `v1.0.0`:
```cmake
set(REQUIRED_THREAD_SYSTEM_VERSION "1.0.0")  # Correct
```

**Fixed in**: integrated_thread_system v2.0.0 (2025-11-16, Phase 1 critical fixes)

### Issue: "monitoring_system version mismatch"

**Symptom**: Runtime warnings about missing adaptive monitoring features

**Cause**: Using monitoring_system v1.0.x instead of v2.0.0

**Solution**: Update monitoring_system to v2.0.0:
```bash
cd ~/Sources/monitoring_system
git fetch --tags
git checkout v2.0.0
cmake -B build && cmake --build build
```

### Issue: "common_system event bus not found"

**Symptom**: Compilation error for event bus headers

**Cause**: Using common_system v0.9.x (event bus not standalone)

**Solution**: Update common_system to v1.0.0:
```bash
cd ~/Sources/common_system
git fetch --tags
git checkout v1.0.0
```

---

## Verification Commands

### Check Installed Versions

```bash
# Check thread_system version
grep "VERSION" ~/Sources/thread_system/CMakeLists.txt | head -1

# Check logger_system version
grep "VERSION" ~/Sources/logger_system/CMakeLists.txt | head -1

# Check monitoring_system version
grep "VERSION" ~/Sources/monitoring_system/CMakeLists.txt | head -1

# Check common_system version
grep "VERSION" ~/Sources/common_system/CMakeLists.txt | head -1
```

### Verify Build Configuration

```bash
cd ~/Sources/integrated_thread_system
cmake -B build 2>&1 | grep "version requirements"
```

Expected output:
```
-- Dependency version requirements:
--   common_system >= 1.0.0
--   thread_system >= 1.0.0
--   logger_system >= 1.0.0
--   monitoring_system >= 2.0.0
```

---

## See Also

- [README.md](../README.md) - Project overview with current dependencies
- [DEPENDENCIES.md](../DEPENDENCIES.md) - Detailed dependency installation guide
- [CHANGELOG.md](../CHANGELOG.md) - Version history and breaking changes
- [MIGRATION.md](../MIGRATION.md) - Migration guide from individual systems

---

**Document Version**: 1.0.0
**Last Updated**: 2025-11-16
**Maintained By**: kcenon

