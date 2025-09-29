/**
 * @file 01_basic_improved.cpp
 * @brief Basic example with all improvements from thread_system, logger_system, and monitoring_system
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

// Color codes for better output
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

void print_section(const std::string& title) {
    std::cout << "\n" << BLUE << "━━━ " << title << " ━━━" << RESET << "\n";
}

int main() {
    std::cout << CYAN << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Integrated Thread System - Improved Example  ║\n";
    std::cout << "╚════════════════════════════════════════════════╝" << RESET << "\n";

    // Create system with custom configuration (from thread_system)
    unified_thread_system::config cfg;
    cfg.name = "ImprovedSystem";
    cfg.thread_count = 4;  // Explicit thread count
    cfg.enable_console_logging = true;
    cfg.enable_file_logging = false;
    cfg.min_log_level = log_level::info;

    unified_thread_system system(cfg);

    print_section("1. Basic Task Execution");
    {
        // Simple task submission
        auto future = system.submit([]() {
            std::cout << GREEN << "✓ Task executing on worker thread" << RESET << std::endl;
            return 42;
        });

        int result = future.get();
        std::cout << "Result: " << result << std::endl;

        // Log the result (from logger_system)
        system.log(log_level::info, "Task completed with result: " + std::to_string(result));
    }

    print_section("2. Batch Processing (Enhanced)");
    {
        std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8};

        auto start = std::chrono::steady_clock::now();

        auto futures = system.submit_batch(
            data.begin(), data.end(),
            [](int x) {
                std::this_thread::sleep_for(50ms);
                return x * x;
            }
        );

        std::cout << "Processing " << data.size() << " items in parallel...\n";

        int sum = 0;
        for (auto& future : futures) {
            sum += future.get();
        }

        auto duration = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        std::cout << GREEN << "✓ Sum of squares: " << sum << RESET << std::endl;
        std::cout << GREEN << "✓ Time taken: " << ms << "ms" << RESET;
        std::cout << " (vs " << (data.size() * 50) << "ms sequential)" << std::endl;

        double speedup = static_cast<double>(data.size() * 50) / ms;
        std::cout << GREEN << "✓ Speedup: " << std::fixed << std::setprecision(2)
                  << speedup << "x" << RESET << std::endl;
    }

    print_section("3. Performance Monitoring (from monitoring_system)");
    {
        // Submit various tasks to generate metrics
        for (int i = 0; i < 10; ++i) {
            system.submit([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(i * 5));
                if (i == 7) throw std::runtime_error("Simulated error");
                return i;
            });
        }

        system.wait_for_completion();

        auto metrics = system.get_metrics();

        std::cout << YELLOW << "Performance Metrics:" << RESET << std::endl;
        std::cout << "  • Tasks submitted: " << metrics.tasks_submitted << std::endl;
        std::cout << "  • Tasks completed: " << metrics.tasks_completed << std::endl;
        std::cout << "  • Tasks failed: " << metrics.tasks_failed << std::endl;
        std::cout << "  • Average latency: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(metrics.average_latency).count()
                  << "μs" << std::endl;
        std::cout << "  • Active workers: " << metrics.active_workers << std::endl;
        std::cout << "  • Queue size: " << metrics.queue_size << std::endl;
    }

    print_section("4. System Health Monitoring");
    {
        auto health = system.get_health();

        std::string health_icon;
        std::string health_color;
        switch (health.overall_health) {
            case health_level::healthy:
                health_icon = "✅";
                health_color = GREEN;
                break;
            case health_level::degraded:
                health_icon = "⚠️";
                health_color = YELLOW;
                break;
            case health_level::critical:
            case health_level::failed:
                health_icon = "❌";
                health_color = "\033[31m";  // RED
                break;
        }

        std::cout << health_color << "System Health: " << health_icon << RESET << std::endl;
        std::cout << "  • CPU usage: " << health.cpu_usage_percent << "%" << std::endl;
        std::cout << "  • Memory usage: " << health.memory_usage_percent << "%" << std::endl;
        std::cout << "  • Queue utilization: " << health.queue_utilization_percent << "%" << std::endl;

        if (!health.issues.empty()) {
            std::cout << YELLOW << "Issues detected:" << RESET << std::endl;
            for (const auto& issue : health.issues) {
                std::cout << "  ⚠ " << issue << std::endl;
            }
        }
    }

    print_section("5. Logging Integration (from logger_system)");
    {
        // Different log levels
        system.log(log_level::trace, "This is a trace message");
        system.log(log_level::debug, "Debug information here");
        system.log(log_level::info, "Normal information message");
        system.log(log_level::warning, "Warning: Queue utilization high");
        system.log(log_level::error, "Error: Task failed");
        system.log(log_level::critical, "Critical: System overloaded");

        std::cout << GREEN << "✓ Logging at multiple levels demonstrated" << RESET << std::endl;
    }

    print_section("6. Advanced Patterns");
    {
        // Map-Reduce pattern (inspired by thread_system's advanced features)
        std::cout << "Map-Reduce example:\n";
        std::vector<int> numbers = {1, 2, 3, 4, 5};

        // Map: square each number, Reduce: sum them
        auto map_futures = system.submit_batch(
            numbers.begin(), numbers.end(),
            [](int x) { return x * x; }
        );

        int total = 0;
        for (auto& f : map_futures) {
            total += f.get();
        }

        std::cout << "  Map (square): [1,2,3,4,5] → [1,4,9,16,25]\n";
        std::cout << "  Reduce (sum): " << total << std::endl;

        // Pipeline pattern
        std::cout << "\nPipeline pattern:\n";
        auto stage1 = system.submit([]() { return 10; });
        auto stage2 = system.submit([&stage1]() { return stage1.get() * 2; });
        auto stage3 = system.submit([&stage2]() { return stage2.get() + 5; });

        std::cout << "  Stage 1: 10\n";
        std::cout << "  Stage 2: × 2 = 20\n";
        std::cout << "  Stage 3: + 5 = " << stage3.get() << std::endl;
    }

    print_section("7. Error Handling & Recovery");
    {
        // Submit task that may fail
        auto risky_future = system.submit([]() -> int {
            static int counter = 0;
            if (++counter % 2 == 0) {
                throw std::runtime_error("Simulated failure");
            }
            return counter;
        });

        try {
            auto result = risky_future.get();
            std::cout << GREEN << "✓ Task succeeded with result: " << result << RESET << std::endl;
        } catch (const std::exception& e) {
            std::cout << YELLOW << "✓ Error handled gracefully: " << e.what() << RESET << std::endl;
        }

        // System continues to work after error
        auto recovery = system.submit([]() {
            return std::string("System recovered and operational");
        });
        std::cout << GREEN << "✓ " << recovery.get() << RESET << std::endl;
    }

    print_section("8. Integration Benefits Summary");
    {
        std::cout << GREEN << "✅ Thread System Benefits:" << RESET << std::endl;
        std::cout << "   • Efficient thread pool with " << system.worker_count() << " workers\n";
        std::cout << "   • Zero-configuration setup\n";
        std::cout << "   • Batch processing support\n";

        std::cout << GREEN << "\n✅ Logger System Benefits:" << RESET << std::endl;
        std::cout << "   • Multiple log levels (trace → critical)\n";
        std::cout << "   • Automatic task logging\n";
        std::cout << "   • Configurable output destinations\n";

        std::cout << GREEN << "\n✅ Monitoring System Benefits:" << RESET << std::endl;
        std::cout << "   • Real-time performance metrics\n";
        std::cout << "   • System health monitoring\n";
        std::cout << "   • Automatic error tracking\n";

        std::cout << GREEN << "\n✅ Common System Benefits:" << RESET << std::endl;
        std::cout << "   • Unified interface across all systems\n";
        std::cout << "   • Consistent error handling\n";
        std::cout << "   • Seamless integration\n";
    }

    std::cout << CYAN << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║         All Examples Completed Successfully!    ║\n";
    std::cout << "╚════════════════════════════════════════════════╝" << RESET << "\n\n";

    return 0;
}