# Dependencies

This document describes the external dependencies required by integrated_thread_system and their version requirements.

## Required System Dependencies

integrated_thread_system v2.0.0 requires the following external systems to be available:

| System | Required Version | Purpose | Repository |
|--------|-----------------|---------|------------|
| **common_system** | v1.0.0+ | Result<T> pattern, error codes, standalone event bus | [kcenon/common_system](https://github.com/kcenon/common_system) |
| **thread_system** | v1.0.0+ | Thread pool, scheduler, service registry, crash handler | [kcenon/thread_system](https://github.com/kcenon/thread_system) |
| **logger_system** | v1.0.0+ | Backend architecture, formatters, async logging | [kcenon/logger_system](https://github.com/kcenon/logger_system) |
| **monitoring_system** | v2.0.0+ | Adaptive monitoring, health checks, reliability features | [kcenon/monitoring_system](https://github.com/kcenon/monitoring_system) |

> **Note**: See [VERSION_COMPATIBILITY.md](docs/VERSION_COMPATIBILITY.md) for detailed compatibility matrix and upgrade paths.

## Library Dependencies

| Library | Purpose | Type |
|---------|---------|------|
| **nlohmann_json** | JSON serialization/deserialization | Header-only (private) |
| **fmt** | Formatting (legacy support) | Optional |
| **Google Test** | Unit testing framework | Optional (BUILD_TESTS) |
| **Google Benchmark** | Performance benchmarking | Optional (BUILD_BENCHMARKS) |

### Optional vcpkg Features

The project supports additional features via vcpkg:

| Feature | Dependencies | Purpose |
|---------|--------------|---------|
| `testing` | GTest, GMock | Unit testing infrastructure |
| `benchmarking` | Google Benchmark | Performance benchmarking |
| `web-dashboard` | RestInIO, WebSocketpp, OpenSSL | Web-based monitoring dashboard |
| `network-logging` | ASIO | Network-based log sinks |
| `database-storage` | SQLite3, PostgreSQL | Metrics persistence |
| `cloud-integration` | AWS SDK | Cloud monitoring integration |
| `compression` | LZ4, Zstd, Snappy | Log compression |

To enable a feature:
```bash
vcpkg install integrated-thread-system[testing,benchmarking]
```

## Compiler Requirements

- **C++ Standard**: C++20 (required for `std::jthread`, concepts, ranges)
- **Compiler Support**:
  - GCC 11+ (full C++20 support)
  - Clang 14+ (full C++20 support)
  - MSVC 19.29+ (Visual Studio 2019 16.10+)
  - Apple Clang 14.0+ (Xcode 14+)

> **Note**: Full `std::format` support requires GCC 13+, Clang 15+. Earlier compiler versions may fall back to `fmt` library if available.

## Installation

### Method 1: Local Development (Recommended)

Clone all required systems as sibling directories:

```bash
cd ~/Sources  # or your preferred location

# Clone dependencies
git clone https://github.com/kcenon/common_system.git
git clone https://github.com/kcenon/thread_system.git
git clone https://github.com/kcenon/logger_system.git
git clone https://github.com/kcenon/monitoring_system.git

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

## Troubleshooting

### "common_system not found"

Ensure common_system is available in one of the search paths or let FetchContent handle it automatically.

### "Dependency mismatch"

Update the dependency to the latest compatible version:

```bash
cd ~/Sources/common_system
git fetch --tags
git pull origin main
```

### "C++20 not supported"

Upgrade your compiler to a version that supports C++20. See compiler requirements above.

## See Also

- [BUILD.md](BUILD.md) - Build instructions
- [README.md](README.md) - Project overview
- [CHANGELOG.md](CHANGELOG.md) - Version history and breaking changes
