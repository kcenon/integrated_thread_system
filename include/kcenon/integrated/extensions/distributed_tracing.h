// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file distributed_tracing.h
 * @brief Distributed tracing support
 */

#pragma once

#include <memory>
#include <string>
#include <kcenon/common/patterns/result.h>

namespace kcenon::integrated::extensions {

class distributed_tracing {
public:
    distributed_tracing();
    ~distributed_tracing();

    common::VoidResult initialize();
    common::VoidResult shutdown();

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace kcenon::integrated::extensions
