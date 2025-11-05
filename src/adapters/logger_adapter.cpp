// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/logger_adapter.h>

#if EXTERNAL_SYSTEMS_AVAILABLE
// Use external logger_system's logger with builder API
#include <kcenon/logger/core/logger.h>
#include <kcenon/logger/core/logger_builder.h>
#include <kcenon/logger/writers/console_writer.h>
#include <kcenon/logger/writers/file_writer.h>
#include <kcenon/logger/formatters/timestamp_formatter.h>
#include <kcenon/logger/formatters/json_formatter.h>
#else
// Fallback to built-in implementation
#include <iostream>
#include <mutex>
#include <iomanip>
#include <chrono>
#endif

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
            // Use logger_builder for modern API (logger_system v3.0.0+)
            kcenon::logger::logger_builder builder;
            builder.with_async(config_.async_mode)
                   .with_buffer_size(config_.buffer_size);

            // Configure formatter based on format option
            switch (config_.format) {
                case log_format::json: {
                    // JSON formatter for log aggregation systems
                    kcenon::logger::format_options opts;
                    opts.include_timestamp = true;
                    opts.include_thread_id = config_.include_thread_id;
                    opts.include_source_location = config_.include_source_location;
                    opts.pretty_print = config_.pretty_print_json;
                    builder.with_formatter(std::make_unique<kcenon::logger::json_formatter>(opts));
                    break;
                }
                case log_format::timestamp:
                default: {
                    // Default timestamp formatter
                    kcenon::logger::format_options opts;
                    opts.include_timestamp = true;
                    opts.include_thread_id = config_.include_thread_id;
                    opts.include_source_location = config_.include_source_location;
                    // Note: enable_colors is not supported in current logger_system
                    builder.with_formatter(std::make_unique<kcenon::logger::timestamp_formatter>(opts));
                    break;
                }
            }

            // Add console writer if enabled
            if (config_.enable_console_logging) {
                builder.add_writer("console", std::make_unique<kcenon::logger::console_writer>());
            }

            // Add file writer if enabled
            if (config_.enable_file_logging) {
                std::string log_file = config_.log_directory + "/integrated_thread_system.log";
                builder.add_writer("file", std::make_unique<kcenon::logger::file_writer>(log_file));
            }

            // Set minimum log level
            builder.with_min_level(convert_log_level(config_.min_log_level));

            // Use standalone backend
            builder.with_standalone_backend();

            // Build the logger
            auto logger_result = builder.build();
            if (!logger_result) {
                return common::VoidResult::err(
                    common::error_codes::INTERNAL_ERROR,
                    std::string("Failed to build logger: ") + logger_result.error_message()
                );
            }

            logger_ = std::move(logger_result.value());

            // Start the logger
            auto start_result = logger_->start();
            if (!start_result) {
                return common::VoidResult::err(
                    common::error_codes::INTERNAL_ERROR,
                    "Failed to start logger"
                );
            }

            initialized_ = true;
            return common::ok();
#else
            // Built-in console logger (fallback)
            initialized_ = true;
            return common::ok();
#endif
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

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (logger_) {
            flush();
            auto stop_result = logger_->stop();
            if (!stop_result) {
                return common::VoidResult::err(
                    common::error_codes::INTERNAL_ERROR,
                    "Failed to stop logger"
                );
            }
            logger_.reset();
        }
#else
        flush();
#endif

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

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (logger_) {
            logger_->log(convert_log_level(level), message);
        }
#else
        std::lock_guard<std::mutex> lock(log_mutex_);
        print_log(level, message);
#endif
    }

    void log(log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function) {
        if (!initialized_ || level < config_.min_log_level) {
            return;
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        if (logger_) {
            logger_->log(convert_log_level(level), message, file, line, function);
        }
#else
        std::lock_guard<std::mutex> lock(log_mutex_);
        print_log(level, message, file, line, function);
#endif
    }

    void flush() {
#if EXTERNAL_SYSTEMS_AVAILABLE
        if (logger_) {
            logger_->flush();
        }
#else
        std::lock_guard<std::mutex> lock(log_mutex_);
        std::cout.flush();
        std::cerr.flush();
#endif
    }

private:
#if EXTERNAL_SYSTEMS_AVAILABLE
    // Convert integrated log_level to logger_system's log_level
    // Note: logger_system uses thread::log_level when USE_THREAD_SYSTEM_INTEGRATION is defined
    kcenon::logger::log_level convert_log_level(log_level level) const {
        switch (level) {
            case log_level::trace: return kcenon::logger::log_level::trace;
            case log_level::debug: return kcenon::logger::log_level::debug;
            case log_level::info: return kcenon::logger::log_level::info;
            case log_level::warning: return kcenon::logger::log_level::warning;
            case log_level::error: return kcenon::logger::log_level::error;
            case log_level::critical: return kcenon::logger::log_level::critical;
            case log_level::fatal:
                // thread::log_level doesn't have fatal, use critical instead
                return kcenon::logger::log_level::critical;
            default: return kcenon::logger::log_level::info;
        }
    }
#else
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
#endif

    logger_config config_;
    bool initialized_;

#if EXTERNAL_SYSTEMS_AVAILABLE
    // External logger_system integration
    std::unique_ptr<kcenon::logger::logger> logger_;
#else
    // Built-in implementation
    mutable std::mutex log_mutex_;
#endif
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
