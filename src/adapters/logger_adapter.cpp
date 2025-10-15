// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/logger_adapter.h>

// Note: Currently using built-in console logger for simplicity.
// External logger_system integration can be enabled for file output, rotation, and remote logging.
// #if EXTERNAL_SYSTEMS_AVAILABLE
// #include <kcenon/logger/core/logger.h>
// #endif

#include <iostream>
#include <mutex>
#include <iomanip>
#include <chrono>

namespace kcenon::integrated::adapters {

class logger_adapter::impl {
public:
    explicit impl(const logger_config& config)
        : config_(config)
        , initialized_(false) {
    }

    ~impl() {
        if (initialized_) {
            shutdown();
        }
    }

    common::VoidResult initialize() {
        if (initialized_) {
            return common::ok();
        }

        try {
#if EXTERNAL_SYSTEMS_AVAILABLE
            // External logger_system integration would go here
            // Would provide: file output, log rotation, remote logging, structured logging
#endif

            // Built-in console logger
            initialized_ = true;
            return common::ok();
        } catch (const std::exception& e) {
            return common::VoidResult::err(
                common::error_codes::INTERNAL_ERROR,
                std::string("Logger adapter initialization failed: ") + e.what()
            );
        }
    }

    common::VoidResult shutdown() {
        if (!initialized_) {
            return common::ok();
        }

        flush();
        initialized_ = false;
        return common::ok();
    }

    bool is_initialized() const {
        return initialized_;
    }

    void log(log_level level, const std::string& message) {
        if (!initialized_ || level < config_.min_log_level) {
            return;
        }

        std::lock_guard<std::mutex> lock(log_mutex_);
        print_log(level, message);
    }

    void log(log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function) {
        if (!initialized_ || level < config_.min_log_level) {
            return;
        }

        std::lock_guard<std::mutex> lock(log_mutex_);
        print_log(level, message, file, line, function);
    }

    void flush() {
        std::lock_guard<std::mutex> lock(log_mutex_);
        std::cout.flush();
        std::cerr.flush();
    }

private:
    std::string level_to_string(log_level level) const {
        switch (level) {
            case log_level::trace: return "TRACE";
            case log_level::debug: return "DEBUG";
            case log_level::info: return "INFO";
            case log_level::warning: return "WARN";
            case log_level::error: return "ERROR";
            case log_level::critical: return "CRIT";
            case log_level::fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    void print_log(log_level level, const std::string& message,
                   const std::string& file = "", int line = 0,
                   const std::string& function = "") {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostream& out = (level >= log_level::error) ? std::cerr : std::cout;

        out << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count()
            << "] [" << level_to_string(level) << "] ";

        if (!file.empty()) {
            out << "[" << file << ":" << line;
            if (!function.empty()) {
                out << " " << function << "()";
            }
            out << "] ";
        }

        out << message << std::endl;
    }

    logger_config config_;
    bool initialized_;
    mutable std::mutex log_mutex_;
};

// logger_adapter implementation

logger_adapter::logger_adapter(const logger_config& config)
    : pimpl_(std::make_unique<impl>(config)) {
}

logger_adapter::~logger_adapter() = default;

logger_adapter::logger_adapter(logger_adapter&&) noexcept = default;
logger_adapter& logger_adapter::operator=(logger_adapter&&) noexcept = default;

common::VoidResult logger_adapter::initialize() {
    return pimpl_->initialize();
}

common::VoidResult logger_adapter::shutdown() {
    return pimpl_->shutdown();
}

bool logger_adapter::is_initialized() const {
    return pimpl_->is_initialized();
}

void logger_adapter::log(log_level level, const std::string& message) {
    pimpl_->log(level, message);
}

void logger_adapter::log(log_level level, const std::string& message,
                         const std::string& file, int line, const std::string& function) {
    pimpl_->log(level, message, file, line, function);
}

void logger_adapter::flush() {
    pimpl_->flush();
}

} // namespace kcenon::integrated::adapters
