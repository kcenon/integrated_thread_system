/**
 * @file application.h
 * @brief Main application framework for integrated thread system
 */

#pragma once

#include <memory>
#include <string>

// Forward declarations for integrated systems
namespace kcenon::thread::core { class thread_pool; }
namespace kcenon::logger::core { class logger; }
namespace kcenon::monitoring::core { class monitor; }

namespace kcenon::integrated::framework {

/**
 * @brief Main application framework for integrated thread system
 *
 * This class provides a unified interface for managing the thread system,
 * logger system, and monitoring system together.
 */
class application {
public:
    struct config {
        std::string name = "Integrated Application";
        std::string config_file_path = "config/default.json";
        bool enable_thread_system = true;
        bool enable_logger_system = true;
        bool enable_monitoring_system = true;
    };

    explicit application(const config& cfg);
    ~application();

    // Lifecycle management
    bool initialize();
    void run();
    void shutdown();

    // Component access
    std::shared_ptr<thread::core::thread_pool> get_thread_pool();
    std::shared_ptr<logger::core::logger> get_logger();
    std::shared_ptr<monitoring::core::monitor> get_monitor();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace kcenon::integrated::framework
