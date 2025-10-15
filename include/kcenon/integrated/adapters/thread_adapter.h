// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file thread_adapter.h
 * @brief Adapter for thread_system integration
 *
 * Provides a common interface to thread_system's thread pool,
 * abstracting implementation details and enabling dependency injection.
 */

#pragma once

#include <functional>
#include <future>
#include <memory>
#include <kcenon/common/patterns/result.h>
#include <kcenon/integrated/core/configuration.h>

namespace kcenon::integrated::adapters {

/**
 * @brief Adapter for thread_system integration
 *
 * This adapter wraps thread_system's thread pool and provides
 * a unified interface for task execution.
 */
class thread_adapter {
public:
    /**
     * @brief Construct adapter with configuration
     */
    explicit thread_adapter(const thread_config& config);

    /**
     * @brief Destructor ensures proper cleanup
     */
    ~thread_adapter();

    // Non-copyable, movable
    thread_adapter(const thread_adapter&) = delete;
    thread_adapter& operator=(const thread_adapter&) = delete;
    thread_adapter(thread_adapter&&) noexcept;
    thread_adapter& operator=(thread_adapter&&) noexcept;

    /**
     * @brief Initialize the thread pool
     */
    common::VoidResult initialize();

    /**
     * @brief Shutdown the thread pool gracefully
     */
    common::VoidResult shutdown();

    /**
     * @brief Check if thread pool is initialized
     */
    bool is_initialized() const;

    /**
     * @brief Execute a task
     * @param task Task to execute
     * @return Result indicating success or error
     */
    common::VoidResult execute(std::function<void()> task);

    /**
     * @brief Submit a task and get a future
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to execute
     * @param args Arguments to pass
     * @return Future containing the result
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /**
     * @brief Submit a task with priority
     * @tparam F Function type
     * @tparam Args Argument types
     * @param priority Priority level (0-127, higher = more important)
     * @param f Function to execute
     * @param args Arguments to pass
     * @return Future containing the result
     */
    template<typename F, typename... Args>
    auto submit_with_priority(int priority, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /**
     * @brief Get number of worker threads
     */
    std::size_t worker_count() const;

    /**
     * @brief Get current queue size
     */
    std::size_t queue_size() const;

    /**
     * @brief Wait for all tasks to complete
     */
    void wait_for_completion();

    /**
     * @brief Wait for tasks with timeout
     * @param timeout Maximum time to wait
     * @return true if all tasks completed, false if timeout
     */
    bool wait_for_completion_timeout(std::chrono::milliseconds timeout);

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

// Template implementations

template<typename F, typename... Args>
auto thread_adapter::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    auto result = task->get_future();

    execute([task]() { (*task)(); });

    return result;
}

template<typename F, typename... Args>
auto thread_adapter::submit_with_priority(int priority, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    auto result = task->get_future();

    // TODO: Implement priority submission when thread_system supports it
    execute([task]() { (*task)(); });

    return result;
}

} // namespace kcenon::integrated::adapters
