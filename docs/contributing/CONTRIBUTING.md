# Contributing to Integrated Thread System

Thank you for your interest in contributing to integrated_thread_system!

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [Development Workflow](#development-workflow)
4. [Coding Standards](#coding-standards)
5. [Testing Requirements](#testing-requirements)
6. [Pull Request Process](#pull-request-process)

---

## Code of Conduct

Please be respectful and constructive in all interactions. We welcome contributors of all backgrounds and experience levels.

---

## Getting Started

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.16 or higher
- Git

### Building for Development

```bash
git clone https://github.com/kcenon/integrated_thread_system.git
cd integrated_thread_system

# Debug build with tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure
```

---

## Development Workflow

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Make** your changes
4. **Test** your changes thoroughly
5. **Commit** with clear messages
6. **Push** to your fork
7. **Open** a Pull Request

---

## Coding Standards

### Style Guide

- Use clang-format with the provided `.clang-format`
- Follow existing code patterns
- Use meaningful variable and function names
- Comment complex logic

### C++ Guidelines

```cpp
// Good: Clear naming, RAII
auto result = perform_operation();
if (!result) {
    return result.error();
}

// Good: Smart pointers
auto resource = std::make_unique<Resource>();

// Good: Result<T> for error handling
auto load_config(const std::string& path) -> Result<Config>;
```

---

## Testing Requirements

### Required Tests

- Unit tests for new functionality
- Integration tests for component interaction
- Thread safety tests for concurrent code

### Running Tests

```bash
# All tests
ctest --test-dir build

# With sanitizers
cmake -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread"
cmake --build build-tsan
ctest --test-dir build-tsan
```

---

## Pull Request Process

1. Ensure all tests pass
2. Update documentation if needed
3. Add changelog entry
4. Request review from maintainers

### PR Title Format

```
feat: add new feature
fix: resolve issue with X
docs: update API documentation
test: add integration tests
refactor: improve performance
```

---

## Questions?

- Open an [issue](https://github.com/kcenon/integrated_thread_system/issues)
- Email: kcenon@naver.com
