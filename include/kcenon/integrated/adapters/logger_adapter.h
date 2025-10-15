// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file logger_adapter.h
 * @brief Adapter for logger_system integration
 */

#pragma once

#include <string>
#include <memory>
#include <kcenon/common/patterns/result.h>
#include <kcenon/integrated/core/configuration.h>

namespace kcenon::integrated::adapters {

/**
 * @brief Adapter for logger_system integration
 */
class logger_adapter {
public:
    explicit logger_adapter(const logger_config& config);
    ~logger_adapter();

    logger_adapter(const logger_adapter&) = delete;
    logger_adapter& operator=(const logger_adapter&) = delete;
    logger_adapter(logger_adapter&&) noexcept;
    logger_adapter& operator=(logger_adapter&&) noexcept;

    common::VoidResult initialize();
    common::VoidResult shutdown();
    bool is_initialized() const;

    void log(log_level level, const std::string& message);
    void log(log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function);

    void flush();

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace kcenon::integrated::adapters
