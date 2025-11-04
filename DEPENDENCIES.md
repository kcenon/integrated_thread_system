# Dependencies

This document describes the external dependencies required by integrated_thread_system and their version requirements.

## Required System Dependencies

integrated_thread_system requires the following external systems to be available:

| System | Minimum Version | Purpose | Repository |
|--------|----------------|---------|------------|
| **common_system** | v1.0.0 | Result<T> pattern, error codes, interfaces | [kcenon/common_system](https://github.com/kcenon/common_system) |
| **thread_system** | v2.0.0 | Thread pool, cancellation tokens, job queues | [kcenon/thread_system](https://github.com/kcenon/thread_system) |
| **logger_system** | v1.0.0 | Asynchronous logging, formatters, writers | [kcenon/logger_system](https://github.com/kcenon/logger_system) |
| **monitoring_system** | v1.0.0 | Performance profiling, system metrics | [kcenon/monitoring_system](https://github.com/kcenon/monitoring_system) |

## Library Dependencies

| Library | Minimum Version | Purpose | Type |
|---------|----------------|---------|------|
| **nlohmann_json** | v3.11.3 | JSON serialization/deserialization | Header-only |
| **Google Test** | v1.14.0+ | Unit testing framework | Optional (BUILD_TESTS) |
| **Google Benchmark** | v1.8.0+ | Performance benchmarking | Optional (BUILD_BENCHMARKS) |

## Compiler Requirements

- **C++ Standard**: C++20 (required, no fallback)
- **Minimum Compiler Versions**:
  - GCC 10+ (full std::format support in GCC 13+)
  - Clang 14+ (full std::format support in Clang 15+)
  - MSVC 19.29+ (Visual Studio 2019 16.10+)
  - Apple Clang 14.0+ (Xcode 14+)

## Version Compatibility Matrix

### integrated_thread_system v2.0.0 (Current)

| System | Compatible Versions | Tested Versions | Notes |
|--------|-------------------|-----------------|-------|
| common_system | v1.0.0+ | v1.0.0 | C++20 std::format required |
| thread_system | v2.0.0+ | v2.0.0 | Cancellation token support required |
| logger_system | v1.0.0+ | v1.0.0, v3.0.0 | Formatter interface support |
| monitoring_system | v1.0.0+ | v1.0.0 | IMonitor interface support |

### Breaking Changes by Version

#### v2.0.0 (2024-01-15)
- **common_system now REQUIRED** (was optional in v1.x)
- **C++20 now REQUIRED** (was C++17 in v1.x)
- **fmt library removed** (using std::format instead)
- External system integration via adapters
- Result<T> pattern throughout API

#### v1.0.0 (Initial Release)
- Optional common_system support
- C++17 minimum
- fmt library dependency

## Installation

### Method 1: Local Development (Recommended)

Clone all required systems as sibling directories:

```bash
cd ~/Sources  # or your preferred location

# Clone dependencies
git clone --branch v1.0.0 https://github.com/kcenon/common_system.git
git clone --branch v2.0.0 https://github.com/kcenon/thread_system.git
git clone --branch v1.0.0 https://github.com/kcenon/logger_system.git
git clone --branch v1.0.0 https://github.com/kcenon/monitoring_system.git

# Clone integrated_thread_system
git clone https://github.com/kcenon/integrated_thread_system.git

# Build dependencies (thread_system, logger_system require building)
cd thread_system && cmake -B build && cmake --build build
cd ../logger_system && cmake -B build && cmake --build build
cd ../monitoring_system && cmake -B build && cmake --build build

# Build integrated_thread_system
cd ../integrated_thread_system
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Method 2: FetchContent (Automatic)

If dependencies are not found locally, CMake will automatically fetch them from GitHub using FetchContent with the pinned versions.

### Method 3: System Installation

Install dependencies to system paths:

```bash
# Install common_system (header-only)
cd common_system
sudo cmake --install build --prefix /usr/local

# Install thread_system
cd ../thread_system
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build

# Similar for logger_system and monitoring_system
```

## Dependency Search Order

CMake searches for dependencies in the following order:

1. **Environment variable path**: `$HOME/Sources/<system_name>`
2. **Sibling directory**: `../system_name` (relative to integrated_thread_system)
3. **System installation**: `/usr/local/lib/<system_name>`
4. **Custom install prefix**: `${CMAKE_INSTALL_PREFIX}/lib/<system_name>`
5. **FetchContent**: Automatically fetches from GitHub with pinned version

## Version Verification

The build system automatically verifies dependency versions. If an incompatible version is detected, the build will fail with a clear error message:

```
CMake Error: common_system version 0.9.0 is too old. Required: 1.0.0 or newer
```

## Troubleshooting

### "common_system not found"

Ensure common_system is available in one of the search paths or let FetchContent handle it automatically.

### "Version mismatch"

Update the dependency to the required version:

```bash
cd ~/Sources/common_system
git fetch --tags
git checkout v1.0.0
```

### "C++20 not supported"

Upgrade your compiler to a version that supports C++20. See compiler requirements above.

## See Also

- [BUILD.md](BUILD.md) - Build instructions
- [README.md](README.md) - Project overview
- [CHANGELOG.md](CHANGELOG.md) - Version history and breaking changes
