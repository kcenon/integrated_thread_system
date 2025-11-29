# 🚀 Quick Start Guide

Get up and running with Integrated Thread System in **5 minutes**!

## 📋 Prerequisites

- C++17 compatible compiler
- CMake 3.16 or later
- Git

## 🎯 Installation (30 seconds)

### Option 1: Quick Install Script

```bash
git clone https://github.com/your-org/integrated_thread_system
cd integrated_thread_system
./scripts/quick_start.sh dev
```

### Option 2: Manual Build

```bash
git clone https://github.com/your-org/integrated_thread_system
cd integrated_thread_system
mkdir build && cd build
cmake ..
make -j4
```

## 🏃 Your First Program (2 minutes)

### Step 1: Create a file `hello.cpp`

```cpp
#include "unified_thread_system.h"
#include <iostream>

using namespace integrated_thread_system;

int main() {
    // That's it! One line to create the system
    unified_thread_system system;

    // Submit a simple task
    auto future = system.submit([]() {
        return 42;
    });

    // Get the result
    std::cout << "The answer is: " << future.get() << std::endl;

    return 0;
}
```

### Step 2: Compile and Run

```bash
# If you installed the library
g++ -std=c++17 hello.cpp -lintegrated_thread_system -o hello
./hello

# Or with the build directory
g++ -std=c++17 -I../include hello.cpp -L../build/lib -lintegrated_thread_system -o hello
./hello
```

### Expected Output:
```
The answer is: 42
```

**🎉 Congratulations! You've just run your first parallel task!**

## 💡 Quick Examples

### Example 1: Multiple Tasks

```cpp
unified_thread_system system;

// Submit multiple tasks
auto task1 = system.submit([]() { return 1 + 1; });
auto task2 = system.submit([]() { return 2 + 2; });
auto task3 = system.submit([]() { return 3 + 3; });

// Get all results
std::cout << task1.get() << ", " << task2.get() << ", " << task3.get() << std::endl;
// Output: 2, 4, 6
```

### Example 2: Priority Tasks

```cpp
unified_thread_system system;

// Critical task - runs first
system.submit_critical([]() {
    std::cout << "1. Emergency task!" << std::endl;
});

// Normal task - runs second
system.submit([]() {
    std::cout << "2. Regular task" << std::endl;
});

// Background task - runs last
system.submit_background([]() {
    std::cout << "3. Cleanup task" << std::endl;
});
```

### Example 3: Processing Data

```cpp
unified_thread_system system;

std::vector<int> data = {1, 2, 3, 4, 5};

auto future = system.submit([data]() {
    int sum = 0;
    for (int val : data) {
        sum += val * val;  // Square each number
    }
    return sum;
});

std::cout << "Sum of squares: " << future.get() << std::endl;
// Output: Sum of squares: 55
```

## 🔥 Common Use Cases

### Web Server Request Handler
```cpp
void handle_request(const request& req) {
    if (req.is_health_check()) {
        // Health checks need immediate response
        system.submit_critical([req]() {
            return process_health_check(req);
        });
    } else {
        // Regular requests
        system.submit([req]() {
            return process_request(req);
        });
    }
}
```

### Batch Data Processing
```cpp
void process_batch(const std::vector<data>& items) {
    std::vector<std::future<result>> futures;

    for (const auto& item : items) {
        futures.push_back(system.submit([item]() {
            return process_item(item);
        }));
    }

    // Wait for all results
    for (auto& future : futures) {
        handle_result(future.get());
    }
}
```

### Background Cleanup
```cpp
// Schedule periodic cleanup
system.submit_background([]() {
    while (running) {
        std::this_thread::sleep_for(1min);
        cleanup_old_files();
    }
});
```

## 📖 What's Next?

### Beginner Path (15 minutes)
1. ✅ Complete this Quick Start
2. 📚 Read [First Program Tutorial](FIRST_PROGRAM.md)
3. 💻 Try examples in `examples/01_basic/`
4. ❓ Check [FAQ](FAQ.md) for common questions

### Intermediate Path (30 minutes)
1. 🎯 Learn [Priority Scheduling](../guides/PRIORITY_SCHEDULING.md)
2. 🔧 Explore [Error Handling](../guides/ERROR_HANDLING.md)
3. 💻 Try examples in `examples/02_intermediate/`

### Advanced Path (1 hour)
1. ⚡ Study [Performance Tuning](../advanced/PERFORMANCE_TUNING.md)
2. 🏗️ Read [Architecture Guide](../architecture/ARCHITECTURE.md)
3. 💻 Explore `examples/03_advanced/`

## 🆘 Need Help?

- 📖 [Full Documentation](../../README.md)
- 💬 [GitHub Discussions](https://github.com/your-org/integrated_thread_system/discussions)
- 🐛 [Report Issues](https://github.com/your-org/integrated_thread_system/issues)
- 📧 Email: support@integrated-thread-system.org

## 🎯 Key Takeaways

You've learned:
- ✅ How to create a `unified_thread_system` with one line
- ✅ How to submit tasks and get results
- ✅ How to use priority scheduling
- ✅ Basic patterns for common use cases

**Ready for more? Continue with the [First Program Tutorial](FIRST_PROGRAM.md)!**

---

<div align="center">

**Time to complete: 5 minutes** • **Difficulty: Beginner** • **Prerequisites: C++17**

</div>