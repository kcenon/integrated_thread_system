// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/thread_adapter.h>

#if EXTERNAL_SYSTEMS_AVAILABLE
// Use external thread_system's thread_pool
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/cancellation_token.h>
#include <kcenon/thread/interfaces/thread_context.h>

// New adapters and features (thread_system v1.0.0+) - Commented out until API stabilizes
// #include <kcenon/thread/adapters/common_system_executor_adapter.h>
// #include <kcenon/thread/interfaces/scheduler_interface.h>
// #include <kcenon/thread/core/service_registry.h>
// #include <kcenon/thread/interfaces/crash_handler.h>

// Optional features
#ifdef ENABLE_BOUNDED_QUEUE
// #include <kcenon/thread/core/bounded_job_queue.h>
#endif

#ifdef ENABLE_HAZARD_POINTER
// #include <kcenon/thread/core/hazard_pointer.h>
#endif
#else
// Fallback to built-in implementation
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#endif

namespace kcenon::integrated::adapters {

/**
 * @brief Implementation details for thread_adapter
 */
class thread_adapter::impl {
public:
    explicit impl(const thread_config& config)
        : config_(config)
        , initialized_(false)
#if EXTERNAL_SYSTEMS_AVAILABLE
        , scheduler_enabled_(config.enable_scheduler)
        , service_registry_enabled_(config.enable_service_registry)
        , crash_handler_enabled_(config.enable_crash_handler)
#else
        , shutdown_(false)
#endif
    {
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
            // Use external thread_system's thread_pool
            std::size_t thread_count = config_.thread_count;
            if (thread_count == 0) {
                thread_count = std::thread::hardware_concurrency();
                if (thread_count == 0) {
                    thread_count = 4;  // Fallback default
                }
            }

            // Create thread_pool with specified worker count
            thread_pool_ = std::make_shared<kcenon::thread::thread_pool>(
                config_.pool_name.empty() ? "integrated_pool" : config_.pool_name
            );

            // Add worker threads - thread_pool doesn't auto-create workers
            for (std::size_t i = 0; i < thread_count; ++i) {
                auto worker = std::make_unique<kcenon::thread::thread_worker>(
                    true,  // use_time_tag
                    kcenon::thread::thread_context()  // default context
                );
                worker->set_job_queue(thread_pool_->get_job_queue());

                auto enqueue_result = thread_pool_->enqueue(std::move(worker));
                if (enqueue_result.has_error()) {
                    return common::VoidResult::err(
                        common::error_codes::INTERNAL_ERROR,
                        "Failed to add worker"
                    );
                }
            }

            // Now start the thread pool
            auto start_result = thread_pool_->start();
            if (start_result.has_error()) {
                return common::VoidResult::err(
                    common::error_codes::INTERNAL_ERROR,
                    "Failed to start thread pool"
                );
            }

            // TODO: Initialize new features when APIs are stable
            // - common_system_executor_adapter for standard interface
            // - Scheduler interface for delayed/recurring tasks
            // - Service registry for dependency injection
            // - Crash handler for signal-safe recovery

            initialized_ = true;
            return common::ok();
#else
            // Built-in thread pool implementation (fallback)
            std::size_t thread_count = config_.thread_count;
            if (thread_count == 0) {
                thread_count = std::thread::hardware_concurrency();
                if (thread_count == 0) {
                    thread_count = 4;  // Fallback default
                }
            }

            workers_.reserve(thread_count);
            for (std::size_t i = 0; i < thread_count; ++i) {
                workers_.emplace_back([this] { worker_thread(); });
            }

            initialized_ = true;
            return common::ok();
#endif
        } catch (const std::exception& e) {
            return common::VoidResult::err(
                common::error_codes::INTERNAL_ERROR,
                std::string("Thread adapter initialization failed: ") + e.what()
            );
        }
    }

    common::VoidResult shutdown() {
        if (!initialized_) {
            return common::ok();
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Use thread_system's shutdown
        bool success = thread_pool_->shutdown_pool(false);  // Graceful shutdown
        if (!success) {
            return common::VoidResult::err(
                common::error_codes::INTERNAL_ERROR,
                "Failed to stop thread pool"
            );
        }
        thread_pool_.reset();
#else
        // Built-in implementation shutdown
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            shutdown_ = true;
        }
        condition_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers_.clear();
#endif

        initialized_ = false;
        return common::ok();
    }

    bool is_initialized() const {
        return initialized_;
    }

    common::VoidResult execute(std::function<void()> task) {
        if (!initialized_) {
            return common::VoidResult::err(
                common::error_codes::INVALID_ARGUMENT,
                "Thread adapter not initialized"
            );
        }

#if EXTERNAL_SYSTEMS_AVAILABLE
        // Use thread_system's simplified submit_task API
        bool success = thread_pool_->submit_task(std::move(task));
        if (!success) {
            return common::VoidResult::err(
                common::error_codes::INTERNAL_ERROR,
                "Failed to enqueue task"
            );
        }
        return common::ok();
#else
        // Built-in implementation
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            if (shutdown_) {
                return common::VoidResult::err(
                    common::error_codes::INVALID_ARGUMENT,
                    "Thread adapter is shutting down"
                );
            }

            if (config_.max_queue_size > 0 && task_queue_.size() >= config_.max_queue_size) {
                return common::VoidResult::err(
                    common::error_codes::INTERNAL_ERROR,
                    "Task queue is full"
                );
            }

            task_queue_.push(std::move(task));
        }
        condition_.notify_one();

        return common::ok();
#endif
    }

    std::size_t worker_count() const {
#if EXTERNAL_SYSTEMS_AVAILABLE
        return thread_pool_ ? thread_pool_->get_thread_count() : 0;
#else
        return workers_.size();
#endif
    }

    std::size_t queue_size() const {
#if EXTERNAL_SYSTEMS_AVAILABLE
        return thread_pool_ ? thread_pool_->get_pending_task_count() : 0;
#else
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return task_queue_.size();
#endif
    }

    void wait_for_completion() {
#if EXTERNAL_SYSTEMS_AVAILABLE
        // thread_system doesn't have direct wait_for_completion
        // We poll the queue size instead
        while (thread_pool_ && thread_pool_->get_pending_task_count() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
#else
        std::unique_lock<std::mutex> lock(queue_mutex_);
        completion_cv_.wait(lock, [this] {
            return task_queue_.empty() && active_tasks_ == 0;
        });
#endif
    }

    bool wait_for_completion_timeout(std::chrono::milliseconds timeout) {
#if EXTERNAL_SYSTEMS_AVAILABLE
        // Poll-based wait with timeout for thread_system
        auto start = std::chrono::steady_clock::now();
        while (thread_pool_ && thread_pool_->get_pending_task_count() > 0) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
#else
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return completion_cv_.wait_for(lock, timeout, [this] {
            return task_queue_.empty() && active_tasks_ == 0;
        });
#endif
    }

    std::shared_ptr<void> create_cancellation_token() {
#if EXTERNAL_SYSTEMS_AVAILABLE
        auto token = std::make_shared<kcenon::thread::cancellation_token>();
        return std::static_pointer_cast<void>(token);
#else
        // Built-in cancellation token (simple atomic bool)
        return std::make_shared<std::atomic<bool>>(false);
#endif
    }

    void cancel_token(std::shared_ptr<void> token) {
        if (!token) return;

#if EXTERNAL_SYSTEMS_AVAILABLE
        auto cancel_token = std::static_pointer_cast<kcenon::thread::cancellation_token>(token);
        cancel_token->cancel();
#else
        auto atomic_token = std::static_pointer_cast<std::atomic<bool>>(token);
        atomic_token->store(true);
#endif
    }

    bool is_token_cancelled(std::shared_ptr<void> token) const {
        if (!token) return false;

#if EXTERNAL_SYSTEMS_AVAILABLE
        auto cancel_token = std::static_pointer_cast<kcenon::thread::cancellation_token>(token);
        return cancel_token->is_cancelled();
#else
        auto atomic_token = std::static_pointer_cast<std::atomic<bool>>(token);
        return atomic_token->load();
#endif
    }

private:
#if !EXTERNAL_SYSTEMS_AVAILABLE
    void worker_thread() {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] {
                    return shutdown_ || !task_queue_.empty();
                });

                if (shutdown_ && task_queue_.empty()) {
                    return;
                }

                if (!task_queue_.empty()) {
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                    ++active_tasks_;
                }
            }

            if (task) {
                try {
                    task();
                } catch (...) {
                    // Swallow exceptions to prevent worker thread termination
                }

                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    --active_tasks_;
                }
                completion_cv_.notify_all();
            }
        }
    }
#endif

    thread_config config_;
    bool initialized_;

#if EXTERNAL_SYSTEMS_AVAILABLE
    // External thread_system integration
    std::shared_ptr<kcenon::thread::thread_pool> thread_pool_;

    // Feature flags for future v2.0 features
    bool scheduler_enabled_;
    bool service_registry_enabled_;
    bool crash_handler_enabled_;

    // TODO: Add new adapters when APIs are stable
    // std::unique_ptr<kcenon::thread::adapters::common_system_executor_adapter> executor_adapter_;
    // std::shared_ptr<kcenon::thread::interfaces::scheduler_interface> scheduler_;
    // std::shared_ptr<kcenon::thread::core::service_registry> registry_;
#else
    // Built-in implementation
    bool shutdown_;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> task_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable completion_cv_;
    std::size_t active_tasks_ = 0;
#endif
};

// thread_adapter implementation

thread_adapter::thread_adapter(const thread_config& config)
    : pimpl_(std::make_unique<impl>(config)) {
}

thread_adapter::~thread_adapter() = default;

thread_adapter::thread_adapter(thread_adapter&&) noexcept = default;
thread_adapter& thread_adapter::operator=(thread_adapter&&) noexcept = default;

common::VoidResult thread_adapter::initialize() {
    return pimpl_->initialize();
}

common::VoidResult thread_adapter::shutdown() {
    return pimpl_->shutdown();
}

bool thread_adapter::is_initialized() const {
    return pimpl_->is_initialized();
}

common::VoidResult thread_adapter::execute(std::function<void()> task) {
    return pimpl_->execute(std::move(task));
}

std::size_t thread_adapter::worker_count() const {
    return pimpl_->worker_count();
}

std::size_t thread_adapter::queue_size() const {
    return pimpl_->queue_size();
}

void thread_adapter::wait_for_completion() {
    pimpl_->wait_for_completion();
}

bool thread_adapter::wait_for_completion_timeout(std::chrono::milliseconds timeout) {
    return pimpl_->wait_for_completion_timeout(timeout);
}

std::shared_ptr<void> thread_adapter::create_cancellation_token() {
    return pimpl_->create_cancellation_token();
}

void thread_adapter::cancel_token(std::shared_ptr<void> token) {
    pimpl_->cancel_token(token);
}

bool thread_adapter::is_token_cancelled(std::shared_ptr<void> token) const {
    return pimpl_->is_token_cancelled(token);
}

// Scheduler Interface Support

common::Result<std::size_t> thread_adapter::schedule_task(std::function<void()> task,
                                                           std::chrono::milliseconds delay) {
    // TODO: Implement when scheduler_interface API is stable
    return common::Result<std::size_t>::err(
        common::error_codes::INTERNAL_ERROR,
        "Scheduler interface not yet implemented - requires thread_system v2.0+"
    );
}

common::Result<std::size_t> thread_adapter::schedule_recurring_task(
    std::function<void()> task,
    std::chrono::milliseconds initial_delay,
    std::chrono::milliseconds interval) {
    // TODO: Implement when scheduler_interface API is stable
    return common::Result<std::size_t>::err(
        common::error_codes::INTERNAL_ERROR,
        "Recurring scheduler not yet implemented - requires thread_system v2.0+"
    );
}

common::VoidResult thread_adapter::cancel_scheduled_task(std::size_t task_id) {
    // TODO: Implement when scheduler_interface API is stable
    return common::VoidResult::err(
        common::error_codes::INTERNAL_ERROR,
        "Task cancellation not yet implemented - requires thread_system v2.0+"
    );
}

// Feature check methods

bool thread_adapter::is_scheduler_enabled() const {
    // TODO: Implement when scheduler is available
    return false;
}

bool thread_adapter::is_service_registry_enabled() const {
    // TODO: Implement when service registry is available
    return false;
}

bool thread_adapter::is_crash_handler_enabled() const {
    // TODO: Implement when crash handler is available
    return false;
}

} // namespace kcenon::integrated::adapters
