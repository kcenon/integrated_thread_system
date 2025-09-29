/**
 * @file 02_advanced_features.cpp
 * @brief Advanced features demonstration from all integrated systems
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <numeric>
#include <algorithm>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

#define BOLD "\033[1m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

class advanced_examples {
private:
    unified_thread_system& system_;
    std::mt19937 rng_{std::random_device{}()};

public:
    explicit advanced_examples(unified_thread_system& system)
        : system_(system) {}

    void demonstrate_priority_scheduling() {
        std::cout << BLUE << "\n═══ Priority-Based Scheduling (from thread_system) ═══" << RESET << "\n";

        // Note: Priority scheduling would be available in enhanced version
        // This example shows the pattern even with basic version

        struct prioritized_task {
            int priority;
            std::string name;
            std::function<void()> work;
        };

        std::vector<prioritized_task> tasks = {
            {1, "Low Priority", []() {
                std::this_thread::sleep_for(10ms);
                std::cout << "  Low priority task completed\n";
            }},
            {5, "Normal Priority", []() {
                std::this_thread::sleep_for(10ms);
                std::cout << "  Normal priority task completed\n";
            }},
            {10, "High Priority", []() {
                std::this_thread::sleep_for(10ms);
                std::cout << "  High priority task completed\n";
            }}
        };

        // Sort by priority (highest first)
        std::sort(tasks.begin(), tasks.end(),
                  [](const auto& a, const auto& b) { return a.priority > b.priority; });

        std::vector<std::future<void>> futures;
        for (const auto& task : tasks) {
            std::cout << "Submitting: " << task.name << " (priority=" << task.priority << ")\n";
            futures.push_back(system_.submit(task.work));
        }

        for (auto& f : futures) {
            f.wait();
        }

        std::cout << GREEN << "✓ All prioritized tasks completed" << RESET << "\n";
    }

    void demonstrate_circuit_breaker_pattern() {
        std::cout << BLUE << "\n═══ Circuit Breaker Pattern (from monitoring_system) ═══" << RESET << "\n";

        // Simulate a service that might fail
        class Service {
        private:
            int failure_count_ = 0;
            int success_count_ = 0;
            bool circuit_open_ = false;
            static constexpr int failure_threshold_ = 3;

        public:
            std::string call() {
                if (circuit_open_) {
                    throw std::runtime_error("Circuit breaker is open");
                }

                // Simulate random failures
                if (failure_count_ < 3 && (rand() % 4 == 0)) {
                    failure_count_++;
                    if (failure_count_ >= failure_threshold_) {
                        circuit_open_ = true;
                        std::cout << YELLOW << "  ⚠ Circuit breaker opened!" << RESET << "\n";
                    }
                    throw std::runtime_error("Service temporarily unavailable");
                }

                success_count_++;
                return "Service response #" + std::to_string(success_count_);
            }

            void reset() {
                circuit_open_ = false;
                failure_count_ = 0;
                std::cout << GREEN << "  ✓ Circuit breaker reset" << RESET << "\n";
            }

            bool is_open() const { return circuit_open_; }
        };

        Service service;

        // Try calling service multiple times
        for (int i = 0; i < 10; ++i) {
            auto future = system_.submit([&service, i]() -> std::string {
                try {
                    return service.call();
                } catch (const std::exception& e) {
                    return std::string("Call ") + std::to_string(i) + " failed: " + e.what();
                }
            });

            std::cout << "  " << future.get() << "\n";

            if (service.is_open() && i == 5) {
                std::cout << CYAN << "  Waiting before reset..." << RESET << "\n";
                std::this_thread::sleep_for(100ms);
                service.reset();
            }
        }
    }

    void demonstrate_structured_logging() {
        std::cout << BLUE << "\n═══ Structured Logging (from logger_system) ═══" << RESET << "\n";

        // Simulate structured log entries
        struct LogEntry {
            std::string timestamp;
            log_level level;
            std::string component;
            std::string message;
            std::unordered_map<std::string, std::string> fields;
        };

        auto create_timestamp = []() {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            char buffer[100];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&time_t));
            return std::string(buffer);
        };

        std::vector<LogEntry> log_entries = {
            {create_timestamp(), log_level::info, "TaskScheduler",
             "Task submitted", {{"task_id", "1234"}, {"priority", "high"}}},
            {create_timestamp(), log_level::debug, "ThreadPool",
             "Worker assigned", {{"worker_id", "w-02"}, {"queue_size", "5"}}},
            {create_timestamp(), log_level::warning, "MemoryMonitor",
             "Memory usage high", {{"usage_percent", "87"}, {"threshold", "80"}}},
            {create_timestamp(), log_level::error, "TaskExecutor",
             "Task failed", {{"task_id", "1234"}, {"error", "timeout"}, {"retry_count", "3"}}}
        };

        for (const auto& entry : log_entries) {
            // Format structured log
            std::string level_str;
            std::string color;
            switch (entry.level) {
                case log_level::debug: level_str = "DEBUG"; color = CYAN; break;
                case log_level::info: level_str = "INFO"; color = GREEN; break;
                case log_level::warning: level_str = "WARN"; color = YELLOW; break;
                case log_level::error: level_str = "ERROR"; color = "\033[31m"; break;
                default: level_str = "UNKNOWN"; color = RESET;
            }

            std::cout << color << "[" << entry.timestamp << "] "
                      << "[" << level_str << "] "
                      << "[" << entry.component << "] "
                      << entry.message;

            // Add fields
            if (!entry.fields.empty()) {
                std::cout << " {";
                bool first = true;
                for (const auto& [key, value] : entry.fields) {
                    if (!first) std::cout << ", ";
                    std::cout << key << ":" << value;
                    first = false;
                }
                std::cout << "}";
            }
            std::cout << RESET << "\n";

            // Also log through the system
            system_.log(entry.level, entry.component + ": " + entry.message);
        }
    }

    void demonstrate_performance_profiling() {
        std::cout << BLUE << "\n═══ Performance Profiling (from monitoring_system) ═══" << RESET << "\n";

        struct TaskProfile {
            std::string name;
            std::chrono::nanoseconds duration;
            bool success;
        };

        std::vector<TaskProfile> profiles;

        // Profile different types of operations
        std::vector<std::pair<std::string, std::function<void()>>> operations = {
            {"CPU Intensive", []() {
                volatile double result = 0;
                for (int i = 0; i < 1000000; ++i) {
                    result += std::sqrt(i) * std::sin(i);
                }
            }},
            {"Memory Allocation", []() {
                std::vector<std::vector<int>> data;
                for (int i = 0; i < 100; ++i) {
                    data.push_back(std::vector<int>(1000, i));
                }
            }},
            {"I/O Simulation", []() {
                std::this_thread::sleep_for(50ms);
            }},
            {"Parallel Subtasks", []() {
                std::vector<std::thread> threads;
                for (int i = 0; i < 4; ++i) {
                    threads.emplace_back([]() {
                        std::this_thread::sleep_for(10ms);
                    });
                }
                for (auto& t : threads) {
                    t.join();
                }
            }}
        };

        for (const auto& [name, operation] : operations) {
            auto future = system_.submit([name, operation]() -> TaskProfile {
                auto start = std::chrono::high_resolution_clock::now();
                bool success = true;

                try {
                    operation();
                } catch (...) {
                    success = false;
                }

                auto end = std::chrono::high_resolution_clock::now();
                return {name, end - start, success};
            });

            profiles.push_back(future.get());
        }

        // Display profiling results
        std::cout << "\n" << BOLD << "Performance Profile Results:" << RESET << "\n";
        std::cout << "┌─────────────────────┬──────────────┬──────────┐\n";
        std::cout << "│ Operation           │ Duration     │ Status   │\n";
        std::cout << "├─────────────────────┼──────────────┼──────────┤\n";

        for (const auto& profile : profiles) {
            auto ms = std::chrono::duration_cast<std::chrono::microseconds>(profile.duration).count();
            std::string status = profile.success ? "✓ Success" : "✗ Failed";
            std::string color = profile.success ? GREEN : "\033[31m";

            std::cout << "│ " << std::left << std::setw(19) << profile.name
                      << " │ " << std::right << std::setw(9) << ms << "μs"
                      << " │ " << color << std::setw(8) << status << RESET << " │\n";
        }
        std::cout << "└─────────────────────┴──────────────┴──────────┘\n";
    }

    void demonstrate_adaptive_monitoring() {
        std::cout << BLUE << "\n═══ Adaptive Monitoring (from monitoring_system) ═══" << RESET << "\n";

        // Simulate load patterns
        std::cout << "Simulating varying load patterns...\n";

        for (int phase = 0; phase < 3; ++phase) {
            std::string phase_name;
            int task_count;
            int task_delay_ms;

            switch (phase) {
                case 0:
                    phase_name = "Low Load";
                    task_count = 5;
                    task_delay_ms = 100;
                    break;
                case 1:
                    phase_name = "Normal Load";
                    task_count = 20;
                    task_delay_ms = 50;
                    break;
                case 2:
                    phase_name = "High Load";
                    task_count = 50;
                    task_delay_ms = 10;
                    break;
            }

            std::cout << "\n" << YELLOW << "Phase " << (phase + 1)
                      << ": " << phase_name << RESET << "\n";

            auto phase_start = std::chrono::steady_clock::now();

            std::vector<std::future<int>> futures;
            for (int i = 0; i < task_count; ++i) {
                futures.push_back(system_.submit([i, task_delay_ms]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(task_delay_ms));
                    return i;
                }));
            }

            // Monitor while tasks are running
            std::this_thread::sleep_for(50ms);

            auto metrics = system_.get_metrics();
            auto health = system_.get_health();

            std::cout << "  Active workers: " << metrics.active_workers << "\n";
            std::cout << "  Queue size: " << metrics.queue_size << "\n";
            std::cout << "  Queue utilization: " << health.queue_utilization_percent << "%\n";

            // Wait for completion
            for (auto& f : futures) {
                f.get();
            }

            auto phase_duration = std::chrono::steady_clock::now() - phase_start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(phase_duration).count();

            std::cout << GREEN << "  ✓ Phase completed in " << ms << "ms" << RESET << "\n";
        }
    }

    void demonstrate_event_driven_architecture() {
        std::cout << BLUE << "\n═══ Event-Driven Architecture (from common_system) ═══" << RESET << "\n";

        // Simulate event bus pattern
        class EventBus {
        private:
            std::unordered_map<std::string, std::vector<std::function<void(const std::string&)>>> listeners_;

        public:
            void subscribe(const std::string& event_type, std::function<void(const std::string&)> handler) {
                listeners_[event_type].push_back(handler);
            }

            void emit(const std::string& event_type, const std::string& data) {
                if (listeners_.count(event_type)) {
                    for (const auto& handler : listeners_[event_type]) {
                        handler(data);
                    }
                }
            }
        };

        EventBus event_bus;

        // Register event handlers
        event_bus.subscribe("task.started", [](const std::string& data) {
            std::cout << CYAN << "  [EVENT] Task started: " << data << RESET << "\n";
        });

        event_bus.subscribe("task.completed", [](const std::string& data) {
            std::cout << GREEN << "  [EVENT] Task completed: " << data << RESET << "\n";
        });

        event_bus.subscribe("task.failed", [](const std::string& data) {
            std::cout << YELLOW << "  [EVENT] Task failed: " << data << RESET << "\n";
        });

        // Simulate task execution with events
        for (int i = 1; i <= 5; ++i) {
            auto future = system_.submit([&event_bus, i]() {
                std::string task_id = "task-" + std::to_string(i);
                event_bus.emit("task.started", task_id);

                std::this_thread::sleep_for(20ms);

                if (i == 3) {  // Simulate failure
                    event_bus.emit("task.failed", task_id + " (simulated error)");
                } else {
                    event_bus.emit("task.completed", task_id);
                }

                return i;
            });

            future.get();
        }
    }
};

int main() {
    std::cout << MAGENTA << BOLD
              << "\n╔══════════════════════════════════════════════════╗\n"
              << "║    Advanced Features Integration Demonstration    ║\n"
              << "╚══════════════════════════════════════════════════╝"
              << RESET << "\n";

    // Configure system with advanced settings
    unified_thread_system::config cfg;
    cfg.name = "AdvancedSystem";
    cfg.thread_count = std::thread::hardware_concurrency();
    cfg.enable_console_logging = true;
    cfg.min_log_level = log_level::debug;

    unified_thread_system system(cfg);
    advanced_examples examples(system);

    // Run all demonstrations
    examples.demonstrate_priority_scheduling();
    examples.demonstrate_circuit_breaker_pattern();
    examples.demonstrate_structured_logging();
    examples.demonstrate_performance_profiling();
    examples.demonstrate_adaptive_monitoring();
    examples.demonstrate_event_driven_architecture();

    // Final system report
    std::cout << BLUE << "\n═══ Final System Report ═══" << RESET << "\n";

    auto final_metrics = system.get_metrics();
    auto final_health = system.get_health();

    std::cout << "\nTotal Statistics:\n";
    std::cout << "  • Total tasks submitted: " << final_metrics.tasks_submitted << "\n";
    std::cout << "  • Total tasks completed: " << final_metrics.tasks_completed << "\n";
    std::cout << "  • Total tasks failed: " << final_metrics.tasks_failed << "\n";

    std::cout << "\nSystem State:\n";
    std::cout << "  • Worker threads: " << system.worker_count() << "\n";
    std::cout << "  • System healthy: " << (system.is_healthy() ? "Yes ✓" : "No ✗") << "\n";

    std::cout << MAGENTA << BOLD
              << "\n╔══════════════════════════════════════════════════╗\n"
              << "║         All Demonstrations Completed!             ║\n"
              << "╚══════════════════════════════════════════════════╝"
              << RESET << "\n\n";

    return 0;
}