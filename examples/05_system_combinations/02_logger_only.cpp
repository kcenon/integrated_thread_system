/**
 * @file 02_logger_only.cpp
 * @brief Using only the logger_system without threading or monitoring
 * @description Synchronous logging without parallel execution
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <chrono>
#include <fstream>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

/**
 * Logger-only configuration is ideal for:
 * - Simple sequential applications
 * - Debugging tools
 * - Configuration utilities
 * - Log analysis tools
 * - Single-threaded services
 */
class logger_only_demo {
private:
    unified_thread_system system_;

public:
    logger_only_demo() {
        // Configure for logger-only operation
        config cfg;
        cfg.enable_thread_system(false)     // Disable threading
           .enable_logger_system(true)       // Enable logging
           .enable_monitoring_system(false)  // Disable monitoring
           .set_log_level(log_level::debug)
           .set_log_file("application.log")
           .set_log_rotation_size(10 * 1024 * 1024)  // 10MB
           .set_log_retention_days(7);

        system_ = unified_thread_system(cfg);

        std::cout << "=== Logger-Only Configuration ===" << std::endl;
        std::cout << "✗ Thread System: DISABLED" << std::endl;
        std::cout << "✓ Logger System: ENABLED" << std::endl;
        std::cout << "✗ Monitoring System: DISABLED" << std::endl;
        std::cout << "Log file: application.log" << std::endl;
        std::cout << std::endl;
    }

    void demonstrate_log_levels() {
        std::cout << "1. Log Levels Demonstration:" << std::endl;

        // Different log levels
        system_.log_debug("Debug information: Variable x = {}", 42);
        system_.log_info("Application started successfully");
        system_.log_warning("Configuration file not found, using defaults");
        system_.log_error("Failed to connect to database");
        system_.log_critical("System memory critically low");

        std::cout << "   Various log levels written to file" << std::endl;
    }

    void demonstrate_structured_logging() {
        std::cout << "\n2. Structured Logging:" << std::endl;

        // Log with structured data
        struct user_event {
            std::string user_id;
            std::string action;
            std::string resource;
            int duration_ms;
        };

        user_event event{
            "user_123",
            "download",
            "report.pdf",
            1250
        };

        system_.log_info("User action",
            {{"user_id", event.user_id},
             {"action", event.action},
             {"resource", event.resource},
             {"duration_ms", event.duration_ms}});

        // Log application metrics
        system_.log_info("Application metrics",
            {{"memory_mb", 256},
             {"cpu_percent", 45.3},
             {"active_connections", 12},
             {"queue_size", 0}});

        std::cout << "   Structured logs written with metadata" << std::endl;
    }

    void demonstrate_context_logging() {
        std::cout << "\n3. Context-based Logging:" << std::endl;

        // Set logging context for a session
        system_.set_log_context({
            {"session_id", "sess_abc123"},
            {"user", "john_doe"},
            {"ip", "192.168.1.100"}
        });

        // All subsequent logs include context
        system_.log_info("User logged in");
        system_.log_info("Accessed dashboard");
        system_.log_warning("Permission denied for admin panel");
        system_.log_info("User logged out");

        // Clear context
        system_.clear_log_context();

        std::cout << "   Context-aware logs written" << std::endl;
    }

    void demonstrate_performance_logging() {
        std::cout << "\n4. Performance Logging:" << std::endl;

        // Log operation timing
        for (int i = 0; i < 5; ++i) {
            auto operation_start = std::chrono::steady_clock::now();

            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(10 + i * 5));

            auto duration = std::chrono::steady_clock::now() - operation_start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            system_.log_info("Operation completed",
                {{"operation_id", i},
                 {"duration_ms", ms},
                 {"status", "success"}});
        }

        std::cout << "   Performance metrics logged" << std::endl;
    }

    void demonstrate_error_tracking() {
        std::cout << "\n5. Error Tracking:" << std::endl;

        // Simulate error scenarios
        auto try_operation = [this](int id, bool should_fail) {
            try {
                if (should_fail) {
                    throw std::runtime_error("Operation failed: Network timeout");
                }

                system_.log_info("Operation {} succeeded", id);
            } catch (const std::exception& e) {
                system_.log_error("Operation {} failed: {}",
                    id, e.what(),
                    {{"error_type", "runtime_error"},
                     {"retry_possible", true},
                     {"error_code", "NET_TIMEOUT"}});
            }
        };

        try_operation(1, false);
        try_operation(2, true);
        try_operation(3, false);
        try_operation(4, true);

        std::cout << "   Error tracking logs written" << std::endl;
    }

    void demonstrate_audit_logging() {
        std::cout << "\n6. Audit Logging:" << std::endl;

        // Security audit logs
        auto log_security_event = [this](const std::string& event_type,
                                        const std::string& user,
                                        bool success) {
            system_.log_info("AUDIT",
                {{"event_type", event_type},
                 {"user", user},
                 {"success", success},
                 {"timestamp", std::chrono::system_clock::now()},
                 {"ip_address", "10.0.0.1"}});
        };

        log_security_event("login_attempt", "admin", true);
        log_security_event("password_change", "user123", true);
        log_security_event("privilege_escalation", "guest", false);
        log_security_event("data_export", "analyst", true);

        std::cout << "   Audit trail created" << std::endl;
    }

    void demonstrate_log_filtering() {
        std::cout << "\n7. Log Filtering and Queries:" << std::endl;

        // Write various logs for filtering demo
        for (int i = 0; i < 20; ++i) {
            if (i % 3 == 0) {
                system_.log_debug("Debug message {}", i);
            } else if (i % 3 == 1) {
                system_.log_info("Info message {}", i);
            } else {
                system_.log_warning("Warning message {}", i);
            }
        }

        // In a real application, you would query logs
        // Here we demonstrate the concept
        std::cout << "   Logs written for filtering" << std::endl;
        std::cout << "   In production: Use log aggregation tools to query" << std::endl;
    }

    void demonstrate_log_rotation() {
        std::cout << "\n8. Log Rotation:" << std::endl;

        // Simulate log rotation by writing many entries
        for (int i = 0; i < 100; ++i) {
            system_.log_info("Log entry for rotation test",
                {{"entry_number", i},
                 {"data", std::string(1000, 'x')}}); // 1KB per entry
        }

        std::cout << "   Log rotation configured (10MB limit)" << std::endl;
        std::cout << "   Old logs archived automatically" << std::endl;
    }

    void demonstrate_efficiency() {
        std::cout << "\n9. Logger Efficiency Metrics:" << std::endl;

        const int num_logs = 10000;
        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < num_logs; ++i) {
            system_.log_info("Performance test log entry {}", i);
        }

        auto duration = std::chrono::steady_clock::now() - start;
        double logs_per_second = num_logs * 1000.0 /
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        std::cout << "   Logging throughput: " << static_cast<int>(logs_per_second)
                  << " logs/sec" << std::endl;
        std::cout << "   No threading overhead" << std::endl;
        std::cout << "   Synchronous, guaranteed ordering" << std::endl;
    }

    void run_all_demonstrations() {
        demonstrate_log_levels();
        demonstrate_structured_logging();
        demonstrate_context_logging();
        demonstrate_performance_logging();
        demonstrate_error_tracking();
        demonstrate_audit_logging();
        demonstrate_log_filtering();
        demonstrate_log_rotation();
        demonstrate_efficiency();

        std::cout << "\n=== Logger-Only Benefits ===" << std::endl;
        std::cout << "✓ Simple, synchronous logging" << std::endl;
        std::cout << "✓ Guaranteed log ordering" << std::endl;
        std::cout << "✓ No threading complexity" << std::endl;
        std::cout << "✓ Minimal resource usage" << std::endl;
        std::cout << "✓ Perfect for sequential applications" << std::endl;

        // Display sample log content
        std::cout << "\n=== Sample Log Output ===" << std::endl;
        std::ifstream log_file("application.log");
        std::string line;
        int lines_shown = 0;
        while (std::getline(log_file, line) && lines_shown < 5) {
            std::cout << line << std::endl;
            lines_shown++;
        }
        if (lines_shown > 0) {
            std::cout << "... (more in application.log)" << std::endl;
        }
    }
};

int main() {
    try {
        logger_only_demo demo;
        demo.run_all_demonstrations();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * When to use Logger-Only configuration:
 *
 * 1. Command-Line Tools
 *    - Build tools
 *    - Configuration utilities
 *    - Deployment scripts
 *
 * 2. Debugging Applications
 *    - Log analyzers
 *    - Trace collectors
 *    - Error reporters
 *
 * 3. Sequential Services
 *    - Cron jobs
 *    - Batch processors
 *    - Data validators
 *
 * 4. Audit Systems
 *    - Compliance logging
 *    - Security auditing
 *    - Access tracking
 */