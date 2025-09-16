#ifndef KCENON_INTEGRATED_PATTERNS_INTEGRATION_PATTERNS_H
#define KCENON_INTEGRATED_PATTERNS_INTEGRATION_PATTERNS_H

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

#endif // KCENON_INTEGRATED_PATTERNS_INTEGRATION_PATTERNS_H
