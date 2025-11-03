# Examples Guide

## Table of Contents
- [Basic Examples](#basic-examples)
- [Enhanced Features](#enhanced-features)
- [Real-World Scenarios](#real-world-scenarios)
- [Performance Optimization](#performance-optimization)
- [Error Handling](#error-handling)
- [Advanced Patterns](#advanced-patterns)

## Basic Examples

### 1. Hello World
The simplest possible example:

```cpp
#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>

using namespace kcenon::integrated;

int main() {
    unified_thread_system system;

    auto future = system.submit([]() {
        return "Hello from thread!";
    });

    std::cout << future.get() << std::endl;
    return 0;
}
```

### 2. Multiple Tasks
Running multiple tasks concurrently:

```cpp
int main() {
    unified_thread_system system;

    std::vector<std::future<int>> futures;

    for (int i = 0; i < 10; ++i) {
        futures.push_back(system.submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return i * i;
        }));
    }

    for (size_t i = 0; i < futures.size(); ++i) {
        std::cout << "Result " << i << ": " << futures[i].get() << std::endl;
    }

    return 0;
}
```

### 3. Batch Processing
Processing a collection in parallel:

```cpp
int main() {
    unified_thread_system system;

    std::vector<std::string> urls = {
        "http://api1.example.com",
        "http://api2.example.com",
        "http://api3.example.com"
    };

    auto futures = system.submit_batch(urls.begin(), urls.end(),
        [](const std::string& url) {
            // Simulate API call
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return "Response from " + url;
        });

    for (auto& future : futures) {
        std::cout << future.get() << std::endl;
    }

    return 0;
}
```

## Enhanced Features

### 4. Priority-Based Tasks
Managing task priorities for critical operations:

```cpp
int main() {
    config cfg;
    cfg.thread_count = 2;  // Limited workers to show priority effect
    unified_thread_system system(cfg);

    // Submit tasks in reverse priority order
    auto low = system.submit_background([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return "Background task";
    });

    auto normal = system.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return "Normal task";
    });

    auto critical = system.submit_critical([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return "Critical task";
    });

    // Critical task executes first despite being submitted last
    std::cout << critical.get() << " (executed first)" << std::endl;
    std::cout << normal.get() << std::endl;
    std::cout << low.get() << " (executed last)" << std::endl;

    return 0;
}
```

### 5. Cancellable Tasks
Tasks that can be cancelled during execution using integrated thread_system cancellation tokens:

```cpp
int main() {
    unified_thread_system system;

    // Create cancellation token
    auto token = system.create_cancellation_token();

    // Submit cancellable task
    auto future = system.submit_cancellable(token, []() {
        for (int i = 0; i < 100; ++i) {
            // Simulate long-running work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Note: Cancellation is checked before task execution
            // For cooperative cancellation during execution,
            // the task function should check token status periodically
            std::cout << "Processing iteration " << i << std::endl;
        }
        return 100;
    });

    // Cancel after 50ms
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    system.cancel_token(token);

    try {
        int result = future.get();
        std::cout << "Task completed with result: " << result << std::endl;
    } catch (const std::runtime_error& e) {
        // Task was cancelled before execution started
        std::cout << "Task was cancelled: " << e.what() << std::endl;
    }

    return 0;
}
```

**Advanced: Cooperative Cancellation**
For tasks that need to check cancellation status during execution:

```cpp
int main() {
    unified_thread_system system;
    auto token = system.create_cancellation_token();

    auto future = system.submit_cancellable(token, [&system, token]() {
        for (int i = 0; i < 100; ++i) {
            // Check cancellation status during execution
            if (system.is_token_cancelled(token)) {
                std::cout << "Task cancelled at iteration " << i << std::endl;
                return -1;  // Return early on cancellation
            }

            // Do work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return 100;
    });

    // Cancel after 50ms
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    system.cancel_token(token);

    int result = future.get();
    if (result == -1) {
        std::cout << "Task was cooperatively cancelled" << std::endl;
    } else {
        std::cout << "Task completed normally: " << result << std::endl;
    }

    return 0;
}
```

### 6. Scheduled Tasks
Delayed and recurring task execution:

```cpp
int main() {
    unified_thread_system system;

    // Schedule task to run after 1 second
    auto delayed = system.schedule(std::chrono::seconds(1), []() {
        return "Executed after 1 second";
    });

    // Schedule recurring task every 500ms
    auto recurring_id = system.schedule_recurring(
        std::chrono::milliseconds(500),
        []() {
            static int count = 0;
            std::cout << "Tick #" << ++count << std::endl;
        });

    std::cout << delayed.get() << std::endl;

    // Let recurring task run for 2 seconds
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Cancel recurring task
    system.cancel_recurring(recurring_id);

    return 0;
}
```

### 7. Map-Reduce Pattern
Parallel data processing with reduction:

```cpp
int main() {
    unified_thread_system system;

    // Calculate sum of squares from 1 to 1000
    std::vector<int> numbers(1000);
    std::iota(numbers.begin(), numbers.end(), 1);

    auto sum_future = system.map_reduce(
        numbers.begin(), numbers.end(),
        [](int n) { return n * n; },        // Map: square each number
        [](int a, int b) { return a + b; }, // Reduce: sum
        0                                    // Initial value
    );

    std::cout << "Sum of squares 1-1000: " << sum_future.get() << std::endl;

    // Find maximum value after transformation
    auto max_future = system.map_reduce(
        numbers.begin(), numbers.end(),
        [](int n) { return n * 2 + 10; },   // Map: transform
        [](int a, int b) { return std::max(a, b); }, // Reduce: max
        0
    );

    std::cout << "Maximum transformed value: " << max_future.get() << std::endl;

    return 0;
}
```

## Real-World Scenarios

### 8. Web Server Request Handler
Simulating a web server with priority handling:

```cpp
struct Request {
    std::string path;
    std::string method;
    bool is_health_check;
};

class WebServer {
    unified_thread_system system_;

public:
    WebServer() : system_(config{}.set_name("WebServer")) {}

    std::future<std::string> handle_request(const Request& req) {
        if (req.is_health_check) {
            // Health checks are critical
            return system_.submit_critical([req]() {
                return std::string("OK");
            });
        } else if (req.method == "POST") {
            // Mutations are normal priority
            return system_.submit([req]() {
                // Process mutation
                return process_post(req);
            });
        } else {
            // Reads are background priority
            return system_.submit_background([req]() {
                // Process query
                return process_get(req);
            });
        }
    }

private:
    static std::string process_post(const Request& req) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return "POST processed: " + req.path;
    }

    static std::string process_get(const Request& req) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        return "GET processed: " + req.path;
    }
};
```

### 9. Data Pipeline with Circuit Breaker
Processing data with failure protection:

```cpp
class DataPipeline {
    unified_thread_system system_;
    std::atomic<int> failure_count_{0};

public:
    DataPipeline() {
        config cfg;
        cfg.enable_circuit_breaker = true;
        cfg.circuit_breaker_failure_threshold = 3;
        system_ = unified_thread_system(cfg);
    }

    void process_batch(const std::vector<std::string>& items) {
        auto futures = system_.submit_batch(
            items.begin(), items.end(),
            [this](const std::string& item) {
                return process_item(item);
            });

        for (auto& future : futures) {
            try {
                auto result = future.get();
                std::cout << "Processed: " << result << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Processing failed: " << e.what() << std::endl;

                if (system_.is_circuit_open()) {
                    std::cerr << "Circuit breaker opened - stopping batch" << std::endl;
                    break;
                }
            }
        }

        // Reset circuit if needed
        if (system_.is_circuit_open()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            system_.reset_circuit_breaker();
            std::cout << "Circuit breaker reset" << std::endl;
        }
    }

private:
    std::string process_item(const std::string& item) {
        // Simulate failures for demonstration
        if (failure_count_++ % 4 == 3) {
            throw std::runtime_error("Simulated failure");
        }
        return "Processed: " + item;
    }
};
```

### 10. Image Processing Pipeline
Parallel image processing with monitoring:

```cpp
class ImageProcessor {
    unified_thread_system system_;

public:
    ImageProcessor() : system_(config{}.set_name("ImageProcessor")) {}

    void process_images(const std::vector<std::string>& image_paths) {
        auto start = std::chrono::steady_clock::now();

        // Stage 1: Load images
        auto load_futures = system_.submit_batch(
            image_paths.begin(), image_paths.end(),
            [](const std::string& path) {
                return load_image(path);
            });

        // Stage 2: Process images
        std::vector<std::future<ProcessedImage>> process_futures;
        for (auto& future : load_futures) {
            process_futures.push_back(
                system_.submit([&future]() {
                    auto image = future.get();
                    return apply_filters(image);
                })
            );
        }

        // Stage 3: Save results
        std::vector<std::future<bool>> save_futures;
        for (size_t i = 0; i < process_futures.size(); ++i) {
            save_futures.push_back(
                system_.submit([&process_futures, i, &image_paths]() {
                    auto processed = process_futures[i].get();
                    return save_image(processed, get_output_path(image_paths[i]));
                })
            );
        }

        // Wait for completion
        for (auto& future : save_futures) {
            future.get();
        }

        auto duration = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

        // Check performance
        auto metrics = system_.get_metrics();
        std::cout << "Processed " << image_paths.size() << " images in "
                  << ms.count() << "ms" << std::endl;
        std::cout << "Average latency: "
                  << metrics.average_latency.count() / 1000000 << "ms" << std::endl;
        std::cout << "Throughput: "
                  << metrics.tasks_per_second << " tasks/second" << std::endl;
    }

private:
    struct Image { std::vector<uint8_t> data; };
    struct ProcessedImage { std::vector<uint8_t> data; };

    static Image load_image(const std::string& path) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return Image{};
    }

    static ProcessedImage apply_filters(const Image& img) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return ProcessedImage{};
    }

    static bool save_image(const ProcessedImage& img, const std::string& path) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        return true;
    }

    static std::string get_output_path(const std::string& input) {
        return "output_" + input;
    }
};
```

## Performance Optimization

### 11. Optimal Thread Count
Finding the optimal thread count for your workload:

```cpp
void benchmark_thread_counts() {
    std::vector<size_t> thread_counts = {1, 2, 4, 8, 16, 32};
    const int num_tasks = 10000;

    for (size_t count : thread_counts) {
        config cfg;
        cfg.thread_count = count;
        cfg.enable_console_logging = false;

        unified_thread_system system(cfg);

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::future<int>> futures;
        for (int i = 0; i < num_tasks; ++i) {
            futures.push_back(system.submit([i]() {
                // Simulate CPU-bound work
                int sum = 0;
                for (int j = 0; j < 1000; ++j) {
                    sum += i * j;
                }
                return sum;
            }));
        }

        for (auto& future : futures) {
            future.get();
        }

        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

        std::cout << "Threads: " << count
                  << ", Time: " << ms.count() << "ms"
                  << ", Tasks/sec: " << (num_tasks * 1000.0 / ms.count())
                  << std::endl;
    }
}
```

### 12. Work Stealing Comparison
Comparing performance with and without work stealing:

```cpp
void compare_work_stealing() {
    const int num_tasks = 1000;

    // Without work stealing
    {
        config cfg;
        cfg.enable_work_stealing = false;
        unified_thread_system system(cfg);

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::future<void>> futures;
        for (int i = 0; i < num_tasks; ++i) {
            futures.push_back(system.submit([i]() {
                // Variable workload
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(i % 10));
            }));
        }

        for (auto& future : futures) {
            future.get();
        }

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Without work stealing: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << "ms" << std::endl;
    }

    // With work stealing
    {
        config cfg;
        cfg.enable_work_stealing = true;
        unified_thread_system system(cfg);

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::future<void>> futures;
        for (int i = 0; i < num_tasks; ++i) {
            futures.push_back(system.submit([i]() {
                // Variable workload
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(i % 10));
            }));
        }

        for (auto& future : futures) {
            future.get();
        }

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "With work stealing: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << "ms" << std::endl;
    }
}
```

## Error Handling

### 13. Graceful Error Recovery
Handling task failures gracefully:

```cpp
class ResilientProcessor {
    unified_thread_system system_;

public:
    void process_with_retry(const std::vector<std::string>& items) {
        for (const auto& item : items) {
            bool success = false;
            int retries = 0;
            const int max_retries = 3;

            while (!success && retries < max_retries) {
                auto future = system_.submit([item, retries]() {
                    return process_item_with_possible_failure(item, retries);
                });

                try {
                    auto result = future.get();
                    std::cout << "Success: " << result << std::endl;
                    success = true;
                } catch (const std::exception& e) {
                    retries++;
                    std::cerr << "Attempt " << retries
                              << " failed: " << e.what() << std::endl;

                    if (retries < max_retries) {
                        // Exponential backoff
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(100 * (1 << retries)));
                    }
                }
            }

            if (!success) {
                std::cerr << "Failed to process " << item
                          << " after " << max_retries << " attempts" << std::endl;
            }
        }
    }

private:
    static std::string process_item_with_possible_failure(
        const std::string& item, int attempt) {
        // Simulate 30% failure rate
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(1, 10);

        if (dis(gen) <= 3 && attempt == 0) {
            throw std::runtime_error("Simulated failure");
        }

        return "Processed: " + item;
    }
};
```

## Advanced Patterns

### 14. Producer-Consumer Pattern
Implementing producer-consumer with the thread system:

```cpp
class ProducerConsumer {
    unified_thread_system system_;
    std::atomic<bool> producing_{true};
    std::queue<int> buffer_;
    std::mutex buffer_mutex_;
    std::condition_variable cv_;

public:
    void run() {
        // Start producers
        std::vector<std::future<void>> producer_futures;
        for (int i = 0; i < 3; ++i) {
            producer_futures.push_back(
                system_.submit([this, i]() { produce(i); })
            );
        }

        // Start consumers
        std::vector<std::future<int>> consumer_futures;
        for (int i = 0; i < 5; ++i) {
            consumer_futures.push_back(
                system_.submit([this]() { return consume(); })
            );
        }

        // Let it run for a while
        std::this_thread::sleep_for(std::chrono::seconds(5));
        producing_ = false;
        cv_.notify_all();

        // Wait for producers
        for (auto& future : producer_futures) {
            future.get();
        }

        // Get consumer results
        int total_consumed = 0;
        for (auto& future : consumer_futures) {
            total_consumed += future.get();
        }

        std::cout << "Total items consumed: " << total_consumed << std::endl;
    }

private:
    void produce(int producer_id) {
        int count = 0;
        while (producing_) {
            std::unique_lock<std::mutex> lock(buffer_mutex_);
            buffer_.push(producer_id * 1000 + count++);
            cv_.notify_one();

            // Simulate production time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    int consume() {
        int count = 0;
        while (producing_ || !buffer_.empty()) {
            std::unique_lock<std::mutex> lock(buffer_mutex_);
            cv_.wait(lock, [this] {
                return !buffer_.empty() || !producing_;
            });

            if (!buffer_.empty()) {
                buffer_.pop();
                count++;

                // Simulate consumption time
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        return count;
    }
};
```

### 15. Event-Driven Architecture
Using the event system for loosely coupled components:

```cpp
class EventDrivenApp {
    unified_thread_system system_;
    std::vector<size_t> subscriptions_;

public:
    void setup() {
        // Subscribe to user events
        subscriptions_.push_back(
            system_.subscribe_to_events("user_login",
                [this](const std::string& event, const std::any& data) {
                    handle_user_login(std::any_cast<std::string>(data));
                })
        );

        subscriptions_.push_back(
            system_.subscribe_to_events("user_action",
                [this](const std::string& event, const std::any& data) {
                    handle_user_action(std::any_cast<std::string>(data));
                })
        );

        // Subscribe to system events
        subscriptions_.push_back(
            system_.subscribe_to_events("system_alert",
                [this](const std::string& event, const std::any& data) {
                    handle_system_alert(std::any_cast<std::string>(data));
                })
        );
    }

    void cleanup() {
        for (auto id : subscriptions_) {
            system_.unsubscribe_from_events(id);
        }
    }

    void simulate_events() {
        // These would normally come from external sources
        trigger_event("user_login", std::string("user123"));
        trigger_event("user_action", std::string("clicked_button"));
        trigger_event("system_alert", std::string("high_memory"));
    }

private:
    void trigger_event(const std::string& type, const std::any& data) {
        // In real implementation, events would be triggered internally
        // This is just for demonstration
    }

    void handle_user_login(const std::string& user_id) {
        system_.submit_critical([user_id]() {
            std::cout << "Processing login for: " << user_id << std::endl;
            // Load user data, update session, etc.
        });
    }

    void handle_user_action(const std::string& action) {
        system_.submit([action]() {
            std::cout << "Processing action: " << action << std::endl;
            // Log analytics, update state, etc.
        });
    }

    void handle_system_alert(const std::string& alert) {
        system_.submit_critical([alert]() {
            std::cout << "ALERT: " << alert << std::endl;
            // Take corrective action
        });
    }
};
```

## Testing Patterns

### 16. Unit Testing with Thread System
Example test structure:

```cpp
class ThreadSystemTest {
    unified_thread_system system_;

public:
    bool test_basic_submission() {
        auto future = system_.submit([]() { return 42; });
        return future.get() == 42;
    }

    bool test_exception_propagation() {
        auto future = system_.submit([]() {
            throw std::runtime_error("test error");
            return 0;
        });

        try {
            future.get();
            return false;  // Should have thrown
        } catch (const std::runtime_error& e) {
            return std::string(e.what()) == "test error";
        }
    }

    bool test_cancellation() {
        cancellation_token token;
        auto future = system_.submit_cancellable(token, []() {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            return 42;
        });

        token.cancel();

        // Cancelled task should return default value
        return future.get() == 0;
    }

    void run_all_tests() {
        std::cout << "Basic submission: "
                  << (test_basic_submission() ? "PASS" : "FAIL") << std::endl;
        std::cout << "Exception propagation: "
                  << (test_exception_propagation() ? "PASS" : "FAIL") << std::endl;
        std::cout << "Cancellation: "
                  << (test_cancellation() ? "PASS" : "FAIL") << std::endl;
    }
};
```

## Best Practices Summary

1. **Use appropriate priorities** - Reserve critical priority for truly urgent tasks
2. **Handle exceptions** - Always wrap future.get() in try-catch for robust error handling
3. **Monitor performance** - Regularly check metrics to detect performance issues
4. **Batch when possible** - Use submit_batch for collections to reduce overhead
5. **Configure appropriately** - Tune thread count and features for your workload
6. **Clean shutdown** - Let the destructor handle cleanup automatically
7. **Use cancellation tokens** - For long-running tasks that may need interruption
8. **Leverage circuit breakers** - Protect against cascading failures
9. **Profile before optimizing** - Use metrics to identify actual bottlenecks
10. **Test concurrent code** - Write tests that verify thread safety and correctness