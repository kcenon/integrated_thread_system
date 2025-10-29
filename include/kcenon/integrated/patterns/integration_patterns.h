/**
 * @file integration_patterns.h
 * @brief Common integration patterns for the integrated thread system ecosystem
 */

#pragma once

namespace kcenon::integrated::patterns {

/**
 * @brief Common integration patterns for the ecosystem
 */

// Producer-Consumer pattern with monitoring
template<typename T>
class monitored_queue;

// Task with automatic logging
template<typename Func>
class logged_task;

// Performance tracked executor
class tracked_executor;

// Circuit breaker pattern
class circuit_breaker;

// Retry with exponential backoff
template<typename Func>
class retry_policy;

} // namespace kcenon::integrated::patterns
