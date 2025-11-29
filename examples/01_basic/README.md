# Basic Examples

This directory contains beginner-friendly examples demonstrating fundamental usage of the Integrated Thread System.

## Examples Overview

| Example | Description | Difficulty |
|---------|-------------|------------|
| `hello_thread.cpp` | Simplest possible example - submit a single task | Beginner |
| `simple_tasks.cpp` | Various task submission patterns | Beginner |
| `futures_basics.cpp` | Working with futures and async results | Beginner |

## Quick Start

```bash
# Build all basic examples
cmake --build build --target hello_thread simple_tasks futures_basics

# Run examples
./build/examples/01_basic/hello_thread
./build/examples/01_basic/simple_tasks
./build/examples/01_basic/futures_basics
```

## hello_thread.cpp

The simplest possible example - just 10 lines of code to run your first threaded task.

```cpp
#include <kcenon/integrated/unified_thread_system.h>
using namespace kcenon::integrated;

int main() {
    unified_thread_system system;  // Zero configuration!

    auto future = system.submit([]() {
        return std::string("Hello from a thread!");
    });

    std::cout << future.get() << std::endl;
}
```

## simple_tasks.cpp

Demonstrates five common task submission patterns:

1. **Tasks with Return Values** - Compute and return results
2. **Tasks with Parameters** - Pass data via lambda captures
3. **Multiple Independent Tasks** - Run tasks in parallel
4. **Void Tasks** - Tasks without return values
5. **Data Processing Tasks** - Process complex data structures

```cpp
// Task with return value
auto future = system.submit([]() {
    int sum = 0;
    for (int i = 1; i <= 100; ++i) sum += i;
    return sum;
});
std::cout << "Sum: " << future.get() << std::endl;

// Multiple parallel tasks
std::vector<std::future<int>> futures;
for (int i = 0; i < 5; ++i) {
    futures.push_back(system.submit([i]() { return i * i; }));
}
```

## futures_basics.cpp

Learn how to work with futures for asynchronous programming:

- `future.get()` - Block and get result
- `future.wait()` - Wait for completion
- `future.wait_for()` - Wait with timeout
- Exception handling in async tasks

## Key Concepts

### Zero-Configuration Initialization

```cpp
unified_thread_system system;  // Uses smart defaults
```

### Lambda Task Submission

```cpp
auto future = system.submit([captured_data]() {
    // Process captured_data
    return result;
});
```

### Future-Based Results

```cpp
auto future = system.submit(task);
auto result = future.get();  // Blocks until complete
```

## Next Steps

After mastering these basics, explore:

- [02_intermediate](../02_intermediate/) - Priority scheduling, work stealing
- [03_advanced](../03_advanced/) - Custom configurations, advanced patterns
- [monitoring](../monitoring/) - Adaptive monitoring, health checks

## See Also

- [Main README](../../README.md)
- [API Reference](../../docs/API.md)
- [Examples Guide](../../docs/EXAMPLES.md)
