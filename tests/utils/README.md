# Test Infrastructure Improvements (Phase 1-3)

This directory contains enhanced test utilities that address common issues in async system testing, particularly the problems identified in Windows CI test failures.

## Overview

The test infrastructure improvements are organized in three phases:

- **Phase 1**: Critical improvements (enhanced wait mechanisms)
- **Phase 2**: Important improvements (platform awareness, test fixtures, diagnostics)
- **Phase 3**: Enhancements (fallback strategies, exception handling)

## Files

### Core Utilities

#### `test_wait_helper.h` (Phase 1.3)
Enhanced waiting mechanisms with timeout and diagnostic support.

**Key Features:**
- Polling-based condition checking (no fixed sleeps)
- Automatic timeout handling
- Rich diagnostic information on failure
- Support for atomic counters, futures, and custom conditions

**Example:**
```cpp
#include "test_wait_helper.h"

std::atomic<bool> done{false};
// ... start async operation ...

auto result = TestWaitHelper::wait_for(
    [&]() { return done.load(); },
    std::chrono::seconds(5)
);

ASSERT_TRUE(result) << result.to_string();
```

#### `platform_test_config.h` (Phase 2.1)
Platform-specific configuration and feature detection.

**Key Features:**
- Platform detection (Windows/macOS/Linux)
- CI environment detection
- Platform-appropriate timeouts
- Sanitizer detection (TSan, ASan)
- Hardware thread detection

**Example:**
```cpp
#include "platform_test_config.h"

// Use platform-appropriate timeout
auto timeout = PlatformTestConfig::event_delivery_timeout();

// Adjust test size for CI
int num_tasks = PlatformTestConfig::is_ci() ? 100 : 1000;

// Print configuration
std::cout << PlatformTestConfig::configuration_info() << std::endl;
```

#### `test_fixture_base.h` (Phase 2.2)
Base test fixtures with automatic diagnostics and setup/teardown.

**Key Features:**
- Automatic timing of tests
- Platform-aware helper methods
- Diagnostic printing on failure
- Custom setup/teardown hooks

**Example:**
```cpp
#include "test_fixture_base.h"

class MyTest : public ThreadSystemTestFixture<unified_thread_system> {
protected:
    void SetUpSystem() override {
        // Custom setup
    }
};

TEST_F(MyTest, SomeTest) {
    auto result = wait_for([&]() { return condition; });
    ASSERT_TRUE(result) << result.to_string();
}
```

#### `async_test_patterns.h` (Phase 2.3 & 3.1 & 3.2)
Reusable patterns for testing async systems.

**Key Features:**
- Delivery tracing
- Fallback strategies (async → sync)
- Exception safety testing
- High-volume test helpers

**Example:**
```cpp
#include "async_test_patterns.h"

// Trace delivery
auto trace = AsyncTestHelper::trace_delivery(
    [&]() { /* operation */ },
    [&]() { return completed; },
    std::chrono::seconds(5)
);

ASSERT_TRUE(trace.completed) << trace.to_string();
```

## Usage Guide

### Quick Start

1. **Include the utilities in your test:**
```cpp
#include "../utils/test_fixture_base.h"
#include "../utils/test_wait_helper.h"
#include "../utils/platform_test_config.h"
```

2. **Use the test fixture:**
```cpp
class MyTest : public ThreadSystemTestFixture<MySystemType> {
    // Your test code
};
```

3. **Wait for async conditions:**
```cpp
auto result = wait_for([&]() { return condition; });
ASSERT_TRUE(result) << result.to_string();
```

### Common Patterns

#### Pattern 1: Simple Async Operation
```cpp
TEST_F(MyTestFixture, AsyncOperation) {
    std::atomic<bool> completed{false};

    // Start async operation
    system_->submit([&]() {
        do_work();
        completed = true;
    });

    // Wait with diagnostics
    auto result = wait_for([&]() { return completed.load(); });

    ASSERT_TRUE(result) << result.to_string();
}
```

#### Pattern 2: Multiple Tasks with Progress
```cpp
TEST_F(MyTestFixture, MultipleTasks) {
    const int num_tasks = 100;
    std::atomic<int> completed{0};

    for (int i = 0; i < num_tasks; ++i) {
        system_->submit([&]() {
            do_work();
            completed++;

            if (completed % 20 == 0) {
                std::cout << "Progress: " << completed << "/" << num_tasks << std::endl;
            }
        });
    }

    auto result = wait_for_count(completed, num_tasks);
    ASSERT_TRUE(result) << result.to_string();
}
```

#### Pattern 3: Platform-Aware Test
```cpp
TEST(MyTest, PlatformAware) {
    if (!PlatformTestConfig::run_high_volume_tests()) {
        GTEST_SKIP() << "High-volume tests disabled in CI";
    }

    int num_tasks = PlatformTestConfig::high_volume_event_count();
    auto timeout = PlatformTestConfig::event_delivery_timeout();

    // ... test code ...
}
```

#### Pattern 4: Exception Safety
```cpp
TEST_F(MyTestFixture, ExceptionSafety) {
    std::atomic<bool> first_called{false};
    std::atomic<bool> second_called{false};

    // First task throws
    system_->submit([&]() {
        first_called = true;
        throw std::runtime_error("Test exception");
    });

    // Second task should still execute
    system_->submit([&]() {
        second_called = true;
    });

    auto result = wait_for([&]() {
        return first_called.load() && second_called.load();
    });

    ASSERT_TRUE(result) << result.to_string();
}
```

#### Pattern 5: Throughput Measurement
```cpp
TEST_F(MyTestFixture, Throughput) {
    double throughput = measure_throughput(1000, [](int i) {
        // Task work
    });

    print_throughput("MyTest", 1000, throughput);

    ASSERT_GT(throughput, expected_minimum);
}
```

## Best Practices

### DO ✅

1. **Use `wait_for()` instead of `sleep()`**
   ```cpp
   // ✅ Good
   auto result = wait_for([&]() { return condition; });

   // ❌ Bad
   std::this_thread::sleep_for(100ms);
   ```

2. **Use `std::atomic` for shared state**
   ```cpp
   // ✅ Good
   std::atomic<bool> flag{false};

   // ❌ Bad
   bool flag = false;  // Data race!
   ```

3. **Provide rich diagnostic messages**
   ```cpp
   // ✅ Good
   ASSERT_TRUE(result) << "Task failed\n" << result.to_string();

   // ❌ Bad
   ASSERT_TRUE(result);
   ```

4. **Use platform-appropriate timeouts**
   ```cpp
   // ✅ Good
   auto timeout = PlatformTestConfig::event_delivery_timeout();

   // ❌ Bad
   auto timeout = std::chrono::seconds(1);  // Too short for some platforms
   ```

5. **Use test fixtures for common setup**
   ```cpp
   // ✅ Good
   class MyTest : public TestFixtureBase<MySystem> {};

   // ❌ Bad
   TEST(MyTest, Test1) { /* Duplicate setup code */ }
   TEST(MyTest, Test2) { /* Duplicate setup code */ }
   ```

### DON'T ❌

1. **Don't use fixed sleep durations**
   ```cpp
   // ❌ Bad
   std::this_thread::sleep_for(100ms);
   EXPECT_TRUE(condition);  // Flaky!
   ```

2. **Don't ignore platform differences**
   ```cpp
   // ❌ Bad
   const auto TIMEOUT = std::chrono::seconds(1);  // May be too short on Windows
   ```

3. **Don't write tests without timeouts**
   ```cpp
   // ❌ Bad
   while (!condition) {
       // Infinite loop if condition never becomes true!
   }
   ```

4. **Don't skip diagnostic information**
   ```cpp
   // ❌ Bad
   ASSERT_TRUE(flag);  // No context on failure
   ```

## Environment Variables

Control test behavior via environment variables:

- `CI=1` - Enable CI mode (reduced test counts, longer timeouts)
- `SKIP_ASYNC_EVENTBUS_TESTS=1` - Skip async EventBus tests (Windows workaround)
- `FORCE_ASYNC_EVENTBUS_TESTS=1` - Force async EventBus tests on Windows
- `RUN_HIGH_VOLUME_TESTS=1` - Enable high-volume tests in CI
- `TSAN_OPTIONS` - ThreadSanitizer options
- `ASAN_OPTIONS` - AddressSanitizer options

## Troubleshooting

### Test Timeout

**Problem**: Test times out waiting for condition

**Solutions**:
1. Check if async operation actually started
2. Verify condition logic is correct
3. Increase timeout for slow platforms
4. Check diagnostics in failure message

**Example**:
```cpp
auto result = wait_with_diagnostics(
    [&]() { return condition; },
    [&]() {
        std::vector<std::string> diags;
        diags.push_back("Counter: " + std::to_string(counter.load()));
        diags.push_back("Expected: 10");
        return diags;
    }
);

if (!result) {
    std::cerr << result.to_string() << std::endl;  // See diagnostics
}
```

### Flaky Tests

**Problem**: Test passes sometimes, fails sometimes

**Solutions**:
1. Check for data races (use TSan)
2. Verify all shared state is atomic or mutex-protected
3. Don't use fixed sleep durations
4. Use proper wait conditions

### Platform-Specific Failures

**Problem**: Test passes locally but fails in CI

**Solutions**:
1. Use platform-aware timeouts
2. Reduce test size in CI
3. Check if feature is supported on platform
4. Add platform-specific skip conditions

**Example**:
```cpp
if (!PlatformTestConfig::async_eventbus_supported()) {
    GTEST_SKIP() << "Async EventBus not supported on this platform";
}
```

## Examples

See complete examples in:
- `tests/unit/test_basic_operations_improved.cpp` - Improved unit tests
- `tests/examples/test_improvements_demo.cpp` - Comprehensive demo of all features

## Performance

The test utilities are designed to be fast:

- **Polling overhead**: ~10ms per check on Unix, ~50ms on Windows
- **Wait latency**: Typically completes within 10-50ms of condition becoming true
- **Diagnostic overhead**: < 1ms to gather diagnostics on failure
- **No performance impact on passing tests**

## Contributing

When adding new test utilities:

1. Follow the existing patterns
2. Document all public APIs
3. Provide usage examples
4. Test on multiple platforms
5. Add to this README

## Summary

These utilities transform this:

```cpp
// ❌ Old style
TEST(MyTest, Something) {
    bool done = false;  // Data race!

    do_async_work([&] { done = true; });

    std::this_thread::sleep_for(100ms);  // Arbitrary wait

    EXPECT_TRUE(done);  // No diagnostic info
}
```

Into this:

```cpp
// ✅ New style
TEST_F(MyTestFixture, Something) {
    std::atomic<bool> done{false};  // Thread-safe

    do_async_work([&] { done = true; });

    auto result = wait_for([&]() { return done.load(); });  // Proper wait

    ASSERT_TRUE(result) << result.to_string();  // Rich diagnostics
}
```

**Result**: More reliable, faster, and easier to debug tests!
