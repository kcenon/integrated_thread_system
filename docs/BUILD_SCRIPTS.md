# Build Scripts Overview

This document describes the comprehensive build script system for the Integrated Thread System, which follows the proven patterns from `thread_system`, `logger_system`, and `monitoring_system`.

## Script Summary

| Script | Purpose | Usage |
|--------|---------|--------|
| `build.sh` | Main build script with comprehensive options | `./build.sh [options]` |
| `dependency.sh` | Automatic dependency setup and vcpkg configuration | `./dependency.sh [options]` |
| `test.sh` | Comprehensive test runner with multiple test types | `./test.sh [options]` |
| `quick_start.sh` | One-command setup for new users | `./quick_start.sh [mode]` |

## Quick Start for New Users

```bash
# For developers (recommended first run)
./quick_start.sh dev

# For a quick demo
./quick_start.sh demo

# For minimal setup
./quick_start.sh minimal
```

## Detailed Script Usage

### 1. build.sh - Main Build Script

**Key Features:**
- Interactive compiler selection with detailed information
- Cross-platform support (Linux, macOS, Windows/MSYS2)
- Automatic dependency detection and fallback strategies
- Comprehensive build options and sanitizer support
- Unified integration of all three systems (thread, logger, monitoring)

**Common Usage Patterns:**

```bash
# Interactive build (default)
./build.sh

# Automatic build with best compiler
./build.sh --auto

# Debug build with tests
./build.sh --debug --tests

# Release build with benchmarks
./build.sh --release --benchmark --lto

# Build with AddressSanitizer
./build.sh --debug --asan --tests

# Standalone build (no external dependencies)
./build.sh --standalone --lib-only

# Custom compiler and cores
./build.sh --compiler clang++-15 --cores 8 --verbose
```

**Build Targets:**
- `--all` - Build everything (default)
- `--lib-only` - Build only the unified library
- `--examples` - Build example applications
- `--tests` - Build and run unit tests

**Integration Options:**
- `--standalone` - No external system dependencies
- `--with-all` - Enable all external integrations
- `--with-thread` - Enable thread_system integration
- `--with-logger` - Enable logger_system integration
- `--with-monitor` - Enable monitoring_system integration

### 2. dependency.sh - Dependency Setup

**Key Features:**
- Multi-platform package manager detection
- Automatic vcpkg setup and package installation
- System dependency verification
- Configuration file generation

**Usage Patterns:**

```bash
# Full setup (recommended for new systems)
./dependency.sh

# Only setup vcpkg (if system packages are already installed)
./dependency.sh --skip-system

# Verify existing installation
./dependency.sh --verify-only

# Clean reinstall of vcpkg
./dependency.sh --clean-vcpkg
```

**Supported Platforms:**
- **Linux**: Ubuntu/Debian, CentOS/RHEL, Fedora, openSUSE, Arch, Alpine
- **macOS**: Homebrew-based installation
- **Windows**: MSYS2/MinGW64 support

### 3. test.sh - Comprehensive Test Runner

**Key Features:**
- Multiple test categories (unit, integration, performance, stress, memory)
- Sanitizer integration for advanced debugging
- Valgrind support for memory testing
- Code coverage generation
- JUnit XML output for CI/CD

**Usage Patterns:**

```bash
# Run all tests
./test.sh

# Run specific test categories
./test.sh --unit --integration

# Run with sanitizers
./test.sh --sanitizers --memory

# Performance benchmarking
./test.sh --performance --release

# Generate coverage report
./test.sh --unit --coverage

# CI/CD friendly
./test.sh --quiet --junit --timeout 120
```

**Test Categories:**
- **Unit Tests**: Individual component testing
- **Integration Tests**: Cross-component interaction testing
- **Performance Tests**: Benchmark execution and profiling
- **Stress Tests**: High-load and multi-threading scenarios
- **Memory Tests**: Leak detection and memory safety validation

### 4. quick_start.sh - One-Command Setup

**Key Features:**
- Mode-based execution for different use cases
- Automatic end-to-end setup
- User-friendly progress reporting
- Smart defaults for each mode

**Available Modes:**

```bash
# Developer setup (full build with tests)
./quick_start.sh dev

# Demo mode (examples only)
./quick_start.sh demo

# Test mode (comprehensive testing)
./quick_start.sh test

# Benchmark mode (performance optimized)
./quick_start.sh benchmark

# Minimal mode (library only)
./quick_start.sh minimal
```

## Build System Features

### Compiler Support

The build scripts automatically detect and support:

- **GCC**: All versions from 8 to 14+
- **Clang**: All versions from 10 to 19+
- **Apple Clang**: macOS system compiler
- **Intel Compilers**: icc, icpc, icx, icpx
- **MSVC**: Via vcpkg toolchain on Windows

### Advanced Features

**C++ Standard Support:**
```bash
# Force C++17 mode
./build.sh --cpp17

# Force C++20 mode (default)
./build.sh --cpp20

# Force fmt library over std::format
./build.sh --force-fmt
```

**Sanitizer Options:**
```bash
# Individual sanitizers
./build.sh --asan    # AddressSanitizer
./build.sh --tsan    # ThreadSanitizer
./build.sh --ubsan   # UndefinedBehaviorSanitizer
./build.sh --msan    # MemorySanitizer

# All sanitizers
./build.sh --sanitizers
```

**Performance Optimization:**
```bash
# Link Time Optimization
./build.sh --lto --release

# Profiling information
./build.sh --profiling --debug

# Static linking
./build.sh --static
```

### Build System Detection

The scripts automatically detect and use the best available build system:

1. **Ninja** (preferred for speed)
2. **Make** (fallback for compatibility)

Force specific build system:
```bash
./build.sh --ninja   # Force Ninja
./build.sh --make    # Force Make
```

## Integration with External Systems

The build scripts are designed to seamlessly integrate with the three component systems:

### Automatic Detection

The build system automatically detects and integrates:
- `../thread_system` - Core threading framework
- `../logger_system` - Async logging system
- `../monitoring_system` - Real-time monitoring

### Manual Integration

```bash
# Enable specific integrations
./build.sh --with-thread --with-logger --with-monitor

# Or enable all
./build.sh --with-all

# Standalone mode (no external dependencies)
./build.sh --standalone
```

## Error Handling and Fallbacks

### Graceful Degradation

The build scripts implement intelligent fallbacks:

1. **vcpkg Failure**: Automatically fallback to system libraries
2. **Compiler Detection**: Use best available compiler
3. **Build System**: Fallback from Ninja to Make
4. **Dependencies**: Continue with reduced feature set

### Error Recovery

```bash
# Clean rebuild on failure
./build.sh --clean

# Force system libraries (bypass vcpkg)
./build.sh --no-vcpkg

# Verbose output for debugging
./build.sh --verbose
```

## CI/CD Integration

The scripts are designed for automated environments:

```bash
# Non-interactive build
./build.sh --auto --quiet

# CI-friendly testing
./test.sh --all --junit --quiet --timeout 300

# Quick verification build
./quick_start.sh minimal --clean
```

### Environment Variables

The scripts respect common CI environment variables:
- `CC` / `CXX` - Compiler override
- `CMAKE_BUILD_TYPE` - Build type override
- `VERBOSE` - Enable verbose output

## Platform-Specific Optimizations

### macOS
- Automatic core count limiting to avoid memory pressure
- Apple Clang detection and configuration
- Homebrew integration for dependencies

### Linux
- Distribution-specific package manager detection
- ARM64 support with vcpkg compatibility
- Multiple compiler version support

### Windows
- MSYS2/MinGW64 support
- Visual Studio integration via vcpkg
- Windows-specific path handling

## Configuration Files

The build scripts generate configuration files:

- `build_config.sh` - Build preferences and platform settings
- `CMakeCache.txt` - CMake configuration cache
- `compile_commands.json` - For IDE integration

## Best Practices

### For Developers

1. **First Setup**: Use `./quick_start.sh dev`
2. **Daily Development**: Use `./build.sh --debug --tests`
3. **Performance Testing**: Use `./build.sh --release --benchmark`
4. **Memory Debugging**: Use `./build.sh --debug --asan --tests`

### For CI/CD

1. **Verification**: Use `./quick_start.sh test --clean`
2. **Release Building**: Use `./build.sh --release --lto --all`
3. **Cross-Platform**: Test on multiple compiler versions

### For Users

1. **Quick Try**: Use `./quick_start.sh demo`
2. **Library Only**: Use `./quick_start.sh minimal`
3. **Full Features**: Use `./quick_start.sh dev`

## Troubleshooting

### Common Issues

**Build Fails with vcpkg:**
```bash
./build.sh --no-vcpkg  # Use system libraries
```

**Compiler Not Found:**
```bash
./build.sh --list-compilers  # See available compilers
./build.sh --compiler g++    # Specify compiler
```

**Memory Issues on macOS:**
```bash
./build.sh --cores 2  # Reduce parallelism
```

**Permission Issues:**
```bash
chmod +x *.sh  # Make scripts executable
```

### Getting Help

```bash
./build.sh --help       # Build options
./test.sh --help        # Testing options
./dependency.sh --help  # Dependency options
./quick_start.sh --help # Quick start modes
```

## Script Architecture

The scripts follow a consistent architecture pattern:

1. **Color-coded output** for better user experience
2. **Comprehensive argument parsing** with validation
3. **Platform detection** and adaptation
4. **Tool detection** and fallback strategies
5. **Progress reporting** with clear status messages
6. **Error handling** with helpful suggestions
7. **Cleanup routines** for interrupted builds

This unified build system ensures that the Integrated Thread System maintains the same high-quality build experience as the individual component systems while providing the simplified usage that matches the original unified thread_system approach.