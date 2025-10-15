// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file monitoring_adapter.h
 * @brief Adapter for monitoring_system integration
 */

#pragma once

#include <string>
#include <memory>
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/interfaces/monitoring_interface.h>
#include <kcenon/integrated/core/configuration.h>

namespace kcenon::integrated::adapters {

/**
 * @brief Adapter for monitoring_system integration
 */
class monitoring_adapter : public common::interfaces::IMonitor {
public:
    explicit monitoring_adapter(const monitoring_config& config);
    ~monitoring_adapter() override;

    monitoring_adapter(const monitoring_adapter&) = delete;
    monitoring_adapter& operator=(const monitoring_adapter&) = delete;
    monitoring_adapter(monitoring_adapter&&) noexcept;
    monitoring_adapter& operator=(monitoring_adapter&&) noexcept;

    common::VoidResult initialize();
    common::VoidResult shutdown();
    bool is_initialized() const;

    // IMonitor interface implementation
    common::VoidResult record_metric(const std::string& name, double value) override;
    common::VoidResult record_metric(const std::string& name, double value,
                                     const std::unordered_map<std::string, std::string>& tags) override;
    common::Result<common::interfaces::metrics_snapshot> get_metrics() override;
    common::Result<common::interfaces::health_check_result> check_health() override;
    common::VoidResult reset() override;

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace kcenon::integrated::adapters
