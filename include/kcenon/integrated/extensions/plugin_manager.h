// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file plugin_manager.h
 * @brief Plugin management system
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <kcenon/common/patterns/result.h>

namespace kcenon::integrated::extensions {

class plugin_manager {
public:
    plugin_manager();
    ~plugin_manager();

    common::VoidResult initialize();
    common::VoidResult shutdown();

    common::VoidResult load_plugin(const std::string& plugin_path);
    common::VoidResult unload_plugin(const std::string& plugin_name);
    std::vector<std::string> list_plugins() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace kcenon::integrated::extensions
