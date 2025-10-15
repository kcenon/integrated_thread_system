// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file system_coordinator.h
 * @brief System coordinator for managing subsystem interactions
 *
 * Coordinates interactions between thread_system, logger_system, and
 * monitoring_system using adapter pattern.
 */

#pragma once

#include <memory>
#include <kcenon/common/patterns/result.h>
#include <kcenon/integrated/core/configuration.h>

namespace kcenon::integrated {

// Forward declarations
namespace adapters {
    class thread_adapter;
    class logger_adapter;
    class monitoring_adapter;
}

/**
 * @brief System coordinator manages all subsystems
 *
 * This class coordinates the lifecycle and interactions of all subsystems,
 * ensuring proper initialization order and graceful shutdown.
 */
class system_coordinator {
public:
    /**
     * @brief Construct coordinator with configuration
     */
    explicit system_coordinator(const unified_config& config);

    /**
     * @brief Destructor ensures proper shutdown
     */
    ~system_coordinator();

    // Non-copyable, movable
    system_coordinator(const system_coordinator&) = delete;
    system_coordinator& operator=(const system_coordinator&) = delete;
    system_coordinator(system_coordinator&&) noexcept;
    system_coordinator& operator=(system_coordinator&&) noexcept;

    /**
     * @brief Initialize all subsystems
     * @return Result indicating success or error
     */
    common::VoidResult initialize();

    /**
     * @brief Shutdown all subsystems gracefully
     * @return Result indicating success or error
     */
    common::VoidResult shutdown();

    /**
     * @brief Check if coordinator is initialized
     */
    bool is_initialized() const;

    /**
     * @brief Get thread adapter
     */
    adapters::thread_adapter* get_thread_adapter();

    /**
     * @brief Get logger adapter
     */
    adapters::logger_adapter* get_logger_adapter();

    /**
     * @brief Get monitoring adapter
     */
    adapters::monitoring_adapter* get_monitoring_adapter();

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace kcenon::integrated
