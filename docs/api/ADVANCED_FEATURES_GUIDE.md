# Advanced Features Guide

This guide provides comprehensive documentation for the advanced features of the Integrated Thread System, demonstrating how the unified API provides access to all the powerful capabilities of the original thread_system, logger_system, and monitoring_system.

## Table of Contents

1. [Priority-Based Job Scheduling](#priority-based-job-scheduling)
2. [Adaptive Queue Optimization](#adaptive-queue-optimization)
3. [Advanced Configuration Options](#advanced-configuration-options)
4. [Performance Monitoring & Metrics](#performance-monitoring--metrics)
5. [Custom Job Types and Priorities](#custom-job-types-and-priorities)
6. [Batch Processing Strategies](#batch-processing-strategies)
7. [Error Handling and Recovery](#error-handling-and-recovery)
8. [Integration with External Systems](#integration-with-external-systems)
9. [Performance Tuning Guidelines](#performance-tuning-guidelines)
10. [Best Practices](#best-practices)

## Priority-Based Job Scheduling

The Integrated Thread System provides sophisticated priority-based job scheduling through a simplified API that internally uses the powerful `typed_thread_pool` from the original thread_system.

### Basic Priority Levels

```cpp
#include "integrated_thread_system.h"
using namespace integrated_thread_system;

unified_thread_system system;

// Critical priority - processed immediately
auto critical_future = system.submit_critical([]() {
    return process_urgent_request();
});

// Normal priority - standard processing queue
auto normal_future = system.submit([]() {
    return handle_regular_task();
});

// Background priority - processed when resources are available
auto background_future = system.submit_background([]() {
    return cleanup_temporary_files();
});
```

### Real-World Priority Scenarios

#### Web Server Request Processing
```cpp
class web_server_handler {
private:
    unified_thread_system thread_system_;

public:
    void handle_request(const http_request& req) {
        switch (req.get_priority()) {
            case request_priority::health_check:
                // Health checks need immediate response
                thread_system_.submit_critical([req]() {
                    return process_health_check(req);
                });
                break;

            case request_priority::user_facing:
                // User-facing requests get normal priority
                thread_system_.submit([req]() {
                    return process_user_request(req);
                });
                break;

            case request_priority::analytics:
                // Analytics can wait - background priority
                thread_system_.submit_background([req]() {
                    return process_analytics(req);
                });
                break;
        }
    }
};
```

#### Image Processing Service
```cpp
class image_processor {
private:
    unified_thread_system system_;

public:
    std::future<image> process_image(const image_request& req) {
        switch (req.quality_level) {
            case quality::thumbnail:
                // Thumbnails for quick preview - critical
                return system_.submit_critical([req]() {
                    return generate_thumbnail(req.source_image);
                });

            case quality::standard:
                // Standard quality - normal priority
                return system_.submit([req]() {
                    return process_standard_quality(req.source_image);
                });

            case quality::high_resolution:
                // High-res processing is resource intensive - background
                return system_.submit_background([req]() {
                    return process_high_resolution(req.source_image);
                });
        }
    }
};
```

### Monitoring Priority Queue Behavior

```cpp
void demonstrate_priority_ordering() {
    unified_thread_system system;
    std::vector<int> execution_order;
    std::mutex order_mutex;

    // Submit jobs in mixed order
    system.submit_background([&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(3);  // Should execute last
    });

    system.submit_critical([&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(1);  // Should execute first
    });

    system.submit([&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(2);  // Should execute second
    });

    // Wait for all jobs to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // execution_order should be [1, 2, 3] regardless of submission order
    assert(execution_order == std::vector<int>{1, 2, 3});
}
```

## Adaptive Queue Optimization

The system automatically optimizes its internal queue strategy based on contention levels, providing the same adaptive_job_queue capabilities from the original thread_system.

### Enabling Adaptive Optimization

```cpp
config system_config;
system_config.enable_adaptive_optimization(true);
system_config.set_adaptation_threshold(100);  // Jobs per second threshold

unified_thread_system system(system_config);
```

### Understanding Adaptation Behavior

```cpp
void demonstrate_adaptive_behavior() {
    unified_thread_system system(config{}.enable_adaptive_optimization(true));

    // Phase 1: Low contention scenario
    std::cout << "Low contention phase...\n";
    for (int i = 0; i < 50; ++i) {
        system.submit([i]() {
            return i * 2;
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // System adapts to use mutex-based queue for low contention

    // Phase 2: High contention scenario
    std::cout << "High contention phase...\n";
    std::vector<std::thread> producers;

    for (int p = 0; p < 8; ++p) {
        producers.emplace_back([&system, p]() {
            for (int i = 0; i < 200; ++i) {
                system.submit([p, i]() {
                    return p * 1000 + i;
                });
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    // System adapts to use lock-free queue for high contention
}
```

### Manual Optimization Hints

```cpp
// For systems with known contention patterns
config low_contention_config;
low_contention_config.set_optimization_hint(optimization_hint::low_contention);

config high_contention_config;
high_contention_config.set_optimization_hint(optimization_hint::high_contention);

// Let system decide automatically (recommended)
config adaptive_config;
adaptive_config.enable_adaptive_optimization(true);
```

## Advanced Configuration Options

### Complete Configuration Example

```cpp
config advanced_config;

// Thread pool configuration
advanced_config.set_worker_count(std::thread::hardware_concurrency())
               .set_queue_capacity(10000)
               .enable_work_stealing(true);

// Priority configuration
advanced_config.set_priority_levels(5)
               .enable_priority_inheritance(true)
               .set_starvation_prevention(true);

// Adaptive optimization
advanced_config.enable_adaptive_optimization(true)
               .set_adaptation_threshold(500)
               .set_adaptation_interval(std::chrono::seconds(5));

// Monitoring and logging
advanced_config.enable_performance_monitoring(true)
               .set_monitoring_interval(std::chrono::seconds(1))
               .enable_detailed_logging(false);  // Disable for production

// Resource management
advanced_config.set_memory_pool_size(1024 * 1024)  // 1MB
               .enable_memory_recycling(true)
               .set_resource_cleanup_interval(std::chrono::minutes(5));

unified_thread_system system(advanced_config);
```

### Runtime Configuration Updates

```cpp
class dynamic_thread_system {
private:
    unified_thread_system system_;

public:
    void adjust_for_high_load() {
        // Increase worker count during peak hours
        system_.resize_worker_pool(system_.get_worker_count() * 2);

        // Enable aggressive optimization
        system_.set_optimization_mode(optimization_mode::aggressive);

        // Increase monitoring frequency
        system_.set_monitoring_interval(std::chrono::milliseconds(500));
    }

    void adjust_for_low_load() {
        // Reduce worker count to save resources
        system_.resize_worker_pool(std::max(2u, system_.get_worker_count() / 2));

        // Use conservative optimization
        system_.set_optimization_mode(optimization_mode::conservative);

        // Reduce monitoring overhead
        system_.set_monitoring_interval(std::chrono::seconds(5));
    }
};
```

## Performance Monitoring & Metrics

### Built-in Metrics Collection

```cpp
unified_thread_system system(config{}.enable_performance_monitoring(true));

// Get real-time statistics
void print_system_stats() {
    auto stats = system.get_performance_stats();

    std::cout << "Jobs processed: " << stats.total_jobs_processed << "\n"
              << "Average latency: " << stats.average_latency.count() << "ms\n"
              << "Throughput: " << stats.jobs_per_second << " jobs/sec\n"
              << "Queue depth: " << stats.current_queue_depth << "\n"
              << "Worker utilization: " << stats.worker_utilization * 100 << "%\n";
}

// Custom metrics callback
system.set_metrics_callback([](const performance_metrics& metrics) {
    // Send to monitoring system
    monitoring_system::record_metric("thread_pool.throughput", metrics.jobs_per_second);
    monitoring_system::record_metric("thread_pool.latency", metrics.average_latency.count());
    monitoring_system::record_metric("thread_pool.queue_depth", metrics.current_queue_depth);
});
```

### Performance Alerts

```cpp
class performance_monitor {
private:
    unified_thread_system& system_;
    std::atomic<bool> monitoring_active_{true};

public:
    performance_monitor(unified_thread_system& sys) : system_(sys) {
        start_monitoring();
    }

private:
    void start_monitoring() {
        std::thread monitor_thread([this]() {
            while (monitoring_active_) {
                auto stats = system_.get_performance_stats();

                // Check for performance issues
                if (stats.average_latency > std::chrono::milliseconds(100)) {
                    alert_high_latency(stats.average_latency);
                }

                if (stats.current_queue_depth > 1000) {
                    alert_queue_buildup(stats.current_queue_depth);
                }

                if (stats.worker_utilization > 0.95) {
                    alert_high_utilization(stats.worker_utilization);
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
        monitor_thread.detach();
    }

    void alert_high_latency(std::chrono::milliseconds latency) {
        std::cout << "ALERT: High latency detected: " << latency.count() << "ms\n";
        // Consider increasing worker count or optimizing job processing
    }

    void alert_queue_buildup(size_t queue_depth) {
        std::cout << "ALERT: Queue buildup detected: " << queue_depth << " jobs\n";
        // Consider load shedding or scaling up
    }

    void alert_high_utilization(double utilization) {
        std::cout << "ALERT: High worker utilization: " << utilization * 100 << "%\n";
        // Consider adding more workers
    }
};
```

## Custom Job Types and Priorities

### Defining Custom Priority Enums

```cpp
enum class business_priority {
    emergency = 0,      // System emergencies
    customer_facing,    // Customer-visible operations
    internal,          // Internal operations
    maintenance,       // Background maintenance
    analytics         // Data processing
};

// Configure system to use custom priorities
config custom_config;
custom_config.set_custom_priority_levels<business_priority>();

unified_thread_system system(custom_config);

// Submit jobs with custom priorities
system.submit(business_priority::emergency, []() {
    return handle_system_emergency();
});

system.submit(business_priority::customer_facing, []() {
    return process_customer_order();
});

system.submit(business_priority::analytics, []() {
    return generate_daily_report();
});
```

### Complex Job Classification

```cpp
struct job_descriptor {
    enum class category { cpu_intensive, io_bound, network_operation };
    enum class urgency { immediate, normal, deferred };

    category job_category;
    urgency job_urgency;
    std::chrono::milliseconds expected_duration;
    std::string department;
};

class intelligent_job_scheduler {
private:
    unified_thread_system system_;

public:
    template<typename Func>
    auto submit_classified_job(const job_descriptor& desc, Func&& func) {
        // Calculate effective priority based on multiple factors
        int priority_score = calculate_priority_score(desc);

        if (priority_score >= 90) {
            return system_.submit_critical(std::forward<Func>(func));
        } else if (priority_score >= 50) {
            return system_.submit(std::forward<Func>(func));
        } else {
            return system_.submit_background(std::forward<Func>(func));
        }
    }

private:
    int calculate_priority_score(const job_descriptor& desc) {
        int score = 0;

        // Urgency factor
        switch (desc.job_urgency) {
            case job_descriptor::urgency::immediate: score += 50; break;
            case job_descriptor::urgency::normal: score += 25; break;
            case job_descriptor::urgency::deferred: score += 0; break;
        }

        // Duration factor (shorter jobs get higher priority for responsiveness)
        if (desc.expected_duration < std::chrono::milliseconds(10)) {
            score += 30;
        } else if (desc.expected_duration < std::chrono::milliseconds(100)) {
            score += 15;
        }

        // Category factor
        switch (desc.job_category) {
            case job_descriptor::category::network_operation: score += 20; break;
            case job_descriptor::category::io_bound: score += 10; break;
            case job_descriptor::category::cpu_intensive: score += 5; break;
        }

        // Department priority
        if (desc.department == "customer_service") {
            score += 15;
        } else if (desc.department == "operations") {
            score += 10;
        }

        return score;
    }
};
```

## Batch Processing Strategies

### Efficient Batch Submission

```cpp
template<typename Iterator>
void submit_batch_with_priorities(unified_thread_system& system,
                                Iterator begin, Iterator end) {
    // Group items by priority for efficient submission
    std::map<job_priority, std::vector<std::function<void()>>> priority_groups;

    for (auto it = begin; it != end; ++it) {
        auto priority = determine_priority(*it);
        priority_groups[priority].emplace_back([item = *it]() {
            return process_item(item);
        });
    }

    // Submit each priority group as a batch
    for (auto& [priority, jobs] : priority_groups) {
        system.submit_batch(priority, std::move(jobs));
    }
}
```

### Adaptive Batch Sizing

```cpp
class adaptive_batch_processor {
private:
    unified_thread_system system_;
    size_t current_batch_size_ = 100;
    std::chrono::milliseconds last_batch_time_{0};

public:
    template<typename Container>
    void process_items(const Container& items) {
        auto batch_start = std::chrono::steady_clock::now();

        // Process in adaptive batches
        auto it = items.begin();
        while (it != items.end()) {
            auto batch_end = std::min(it + current_batch_size_, items.end());

            std::vector<std::future<void>> batch_futures;
            for (auto batch_it = it; batch_it != batch_end; ++batch_it) {
                batch_futures.push_back(system_.submit([item = *batch_it]() {
                    return process_single_item(item);
                }));
            }

            // Wait for this batch to complete
            for (auto& future : batch_futures) {
                future.wait();
            }

            it = batch_end;
        }

        auto batch_duration = std::chrono::steady_clock::now() - batch_start;
        adapt_batch_size(batch_duration);
    }

private:
    void adapt_batch_size(std::chrono::milliseconds duration) {
        if (duration > last_batch_time_ * 1.2) {
            // Batch is taking longer, reduce size
            current_batch_size_ = std::max(size_t{10}, current_batch_size_ * 8 / 10);
        } else if (duration < last_batch_time_ * 0.8) {
            // Batch is faster, can increase size
            current_batch_size_ = std::min(size_t{1000}, current_batch_size_ * 12 / 10);
        }

        last_batch_time_ = duration;
    }
};
```

## Error Handling and Recovery

### Comprehensive Error Handling

```cpp
class robust_job_executor {
private:
    unified_thread_system system_;
    std::unordered_map<std::string, int> failure_counts_;
    std::mutex failure_mutex_;

public:
    template<typename Func>
    auto submit_with_retry(const std::string& job_id, Func&& func,
                          int max_retries = 3) {
        return system_.submit([this, job_id, func = std::forward<Func>(func), max_retries]() {
            for (int attempt = 0; attempt <= max_retries; ++attempt) {
                try {
                    auto result = func();

                    // Success - reset failure count
                    {
                        std::lock_guard<std::mutex> lock(failure_mutex_);
                        failure_counts_[job_id] = 0;
                    }

                    return result;
                }
                catch (const std::exception& e) {
                    {
                        std::lock_guard<std::mutex> lock(failure_mutex_);
                        failure_counts_[job_id]++;
                    }

                    if (attempt == max_retries) {
                        // Final attempt failed
                        handle_job_failure(job_id, e.what(), attempt + 1);
                        throw;
                    }

                    // Exponential backoff
                    auto backoff_time = std::chrono::milliseconds(100 * (1 << attempt));
                    std::this_thread::sleep_for(backoff_time);
                }
            }

            throw std::runtime_error("Should not reach here");
        });
    }

    void handle_job_failure(const std::string& job_id, const std::string& error,
                           int attempt_count) {
        std::cout << "Job " << job_id << " failed after " << attempt_count
                  << " attempts: " << error << std::endl;

        // Could implement additional failure handling:
        // - Send to dead letter queue
        // - Notify monitoring system
        // - Trigger circuit breaker
    }

    std::map<std::string, int> get_failure_statistics() const {
        std::lock_guard<std::mutex> lock(failure_mutex_);
        return failure_counts_;
    }
};
```

### Circuit Breaker Pattern

```cpp
class circuit_breaker_job_system {
private:
    unified_thread_system system_;
    std::atomic<int> consecutive_failures_{0};
    std::atomic<bool> circuit_open_{false};
    std::atomic<std::chrono::steady_clock::time_point> last_failure_time_;

public:
    template<typename Func>
    auto submit_with_circuit_breaker(Func&& func) {
        return system_.submit([this, func = std::forward<Func>(func)]() {
            // Check if circuit is open
            if (circuit_open_.load()) {
                auto now = std::chrono::steady_clock::now();
                auto last_failure = last_failure_time_.load();

                if (now - last_failure < std::chrono::minutes(1)) {
                    throw std::runtime_error("Circuit breaker is open");
                } else {
                    // Try to close circuit
                    circuit_open_ = false;
                }
            }

            try {
                auto result = func();
                consecutive_failures_ = 0;  // Reset on success
                return result;
            }
            catch (const std::exception& e) {
                int failures = consecutive_failures_.fetch_add(1) + 1;

                if (failures >= 5) {
                    circuit_open_ = true;
                    last_failure_time_ = std::chrono::steady_clock::now();
                }

                throw;
            }
        });
    }
};
```

## Integration with External Systems

### Database Connection Pool Integration

```cpp
class database_job_processor {
private:
    unified_thread_system system_;
    database_connection_pool db_pool_;

public:
    std::future<query_result> execute_query(const std::string& sql,
                                           query_priority priority = query_priority::normal) {
        auto submit_func = [this, sql]() {
            auto connection = db_pool_.acquire_connection();
            return connection->execute(sql);
        };

        switch (priority) {
            case query_priority::critical:
                return system_.submit_critical(submit_func);
            case query_priority::normal:
                return system_.submit(submit_func);
            case query_priority::background:
                return system_.submit_background(submit_func);
        }
    }

    void execute_batch_queries(const std::vector<std::string>& queries) {
        // Group queries by estimated execution time
        auto quick_queries = filter_quick_queries(queries);
        auto complex_queries = filter_complex_queries(queries);

        // Submit quick queries with higher priority
        for (const auto& query : quick_queries) {
            execute_query(query, query_priority::normal);
        }

        // Submit complex queries as background tasks
        for (const auto& query : complex_queries) {
            execute_query(query, query_priority::background);
        }
    }
};
```

### Message Queue Integration

```cpp
class message_queue_worker {
private:
    unified_thread_system system_;
    message_queue& queue_;

public:
    message_queue_worker(message_queue& queue) : queue_(queue) {
        start_message_processing();
    }

private:
    void start_message_processing() {
        system_.submit_background([this]() {
            while (true) {
                auto message = queue_.receive_message();
                if (!message) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                // Determine priority based on message attributes
                auto priority = determine_message_priority(*message);

                // Submit message processing with appropriate priority
                switch (priority) {
                    case message_priority::urgent:
                        system_.submit_critical([msg = *message]() {
                            return process_urgent_message(msg);
                        });
                        break;
                    case message_priority::normal:
                        system_.submit([msg = *message]() {
                            return process_normal_message(msg);
                        });
                        break;
                    case message_priority::bulk:
                        system_.submit_background([msg = *message]() {
                            return process_bulk_message(msg);
                        });
                        break;
                }
            }
        });
    }

    message_priority determine_message_priority(const message& msg) {
        if (msg.has_attribute("urgent") || msg.get_type() == "alert") {
            return message_priority::urgent;
        } else if (msg.get_type() == "batch" || msg.get_type() == "analytics") {
            return message_priority::bulk;
        } else {
            return message_priority::normal;
        }
    }
};
```

## Performance Tuning Guidelines

### System Sizing Recommendations

```cpp
config create_optimized_config_for_workload(workload_characteristics workload) {
    config cfg;

    // Base worker count on workload type
    switch (workload.type) {
        case workload_type::cpu_intensive:
            cfg.set_worker_count(std::thread::hardware_concurrency());
            break;
        case workload_type::io_bound:
            cfg.set_worker_count(std::thread::hardware_concurrency() * 2);
            break;
        case workload_type::mixed:
            cfg.set_worker_count(std::thread::hardware_concurrency() * 1.5);
            break;
    }

    // Queue sizing based on expected load
    if (workload.expected_jobs_per_second > 10000) {
        cfg.set_queue_capacity(50000);
        cfg.enable_adaptive_optimization(true);
        cfg.set_adaptation_threshold(1000);
    } else if (workload.expected_jobs_per_second > 1000) {
        cfg.set_queue_capacity(10000);
        cfg.enable_adaptive_optimization(true);
    } else {
        cfg.set_queue_capacity(1000);
        cfg.set_optimization_hint(optimization_hint::low_contention);
    }

    // Memory optimization
    if (workload.memory_sensitive) {
        cfg.enable_memory_recycling(true);
        cfg.set_memory_pool_size(1024 * 1024);  // 1MB pool
    }

    return cfg;
}
```

### Memory Usage Optimization

```cpp
class memory_efficient_processor {
private:
    unified_thread_system system_;
    std::unique_ptr<memory_pool> job_memory_pool_;

public:
    memory_efficient_processor() {
        // Pre-allocate memory pool for job objects
        job_memory_pool_ = std::make_unique<memory_pool>(
            sizeof(std::function<void()>) * 1000);

        config memory_config;
        memory_config.enable_memory_recycling(true)
                    .set_memory_pool_size(2 * 1024 * 1024)  // 2MB
                    .enable_object_pooling(true);

        system_ = unified_thread_system(memory_config);
    }

    template<typename Func>
    auto submit_memory_efficient(Func&& func) {
        // Use placement new with pre-allocated memory
        auto* job_memory = job_memory_pool_->allocate();

        return system_.submit([func = std::forward<Func>(func), job_memory, this]() {
            try {
                auto result = func();
                job_memory_pool_->deallocate(job_memory);
                return result;
            }
            catch (...) {
                job_memory_pool_->deallocate(job_memory);
                throw;
            }
        });
    }
};
```

## Best Practices

### 1. Priority Assignment Guidelines

```cpp
// ✅ Good: Clear priority assignment based on business impact
enum class business_impact {
    customer_blocking = 0,  // Blocks customer operations
    revenue_affecting,      // Affects revenue generation
    operational,           // Internal operations
    optimization,          // Performance improvements
    maintenance           // Background maintenance
};

// ✅ Good: Consistent priority mapping
job_priority map_business_to_system_priority(business_impact impact) {
    switch (impact) {
        case business_impact::customer_blocking:
            return job_priority::critical;
        case business_impact::revenue_affecting:
        case business_impact::operational:
            return job_priority::normal;
        case business_impact::optimization:
        case business_impact::maintenance:
            return job_priority::background;
    }
}
```

### 2. Resource Management

```cpp
// ✅ Good: RAII for system lifecycle
class application {
private:
    unified_thread_system thread_system_;
    database_connection_pool db_pool_;

public:
    application() : thread_system_(create_system_config()) {
        // System starts automatically
    }

    ~application() {
        // Graceful shutdown - finish current jobs
        thread_system_.shutdown_gracefully(std::chrono::seconds(30));
    }

private:
    config create_system_config() {
        return config{}
            .set_worker_count(get_optimal_worker_count())
            .enable_adaptive_optimization(true)
            .enable_performance_monitoring(true);
    }
};
```

### 3. Error Handling Patterns

```cpp
// ✅ Good: Structured error handling with context
template<typename Func>
auto submit_with_context(const std::string& operation_name, Func&& func) {
    return system.submit([operation_name, func = std::forward<Func>(func)]() {
        try {
            return func();
        }
        catch (const std::exception& e) {
            std::string error_context = format_error_context(operation_name, e.what());
            log_error(error_context);

            // Re-throw with additional context
            throw operation_error(operation_name, e.what());
        }
    });
}
```

### 4. Testing Strategies

```cpp
// ✅ Good: Unit tests for priority behavior
void test_priority_ordering() {
    unified_thread_system system;
    std::vector<int> execution_order;
    std::mutex order_mutex;

    // Submit in reverse priority order
    auto bg_future = system.submit_background([&] {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(3);
    });

    auto normal_future = system.submit([&] {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(2);
    });

    auto critical_future = system.submit_critical([&] {
        std::lock_guard lock(order_mutex);
        execution_order.push_back(1);
    });

    // Wait for all
    critical_future.wait();
    normal_future.wait();
    bg_future.wait();

    // Verify priority ordering
    ASSERT_EQ(execution_order, std::vector<int>({1, 2, 3}));
}
```

### 5. Monitoring and Observability

```cpp
// ✅ Good: Comprehensive monitoring setup
class production_thread_system {
private:
    unified_thread_system system_;
    metrics_collector metrics_;

public:
    production_thread_system()
        : system_(create_production_config()) {
        setup_monitoring();
    }

private:
    void setup_monitoring() {
        // Real-time metrics collection
        system_.set_metrics_callback([this](const performance_metrics& metrics) {
            metrics_.record("jobs_per_second", metrics.jobs_per_second);
            metrics_.record("average_latency_ms", metrics.average_latency.count());
            metrics_.record("queue_depth", metrics.current_queue_depth);
            metrics_.record("worker_utilization", metrics.worker_utilization);
        });

        // Health checks
        system_.set_health_check_callback([this]() {
            return check_system_health();
        });

        // Alerting thresholds
        system_.set_alert_thresholds({
            .max_latency = std::chrono::milliseconds(100),
            .max_queue_depth = 1000,
            .max_utilization = 0.95
        });
    }
};
```

This guide demonstrates how the Integrated Thread System provides all the advanced capabilities of the original thread_system while maintaining the simplicity of a unified API. The examples show real-world usage patterns and best practices for building high-performance, scalable applications.