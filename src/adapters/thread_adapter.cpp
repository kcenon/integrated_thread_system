// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/integrated/adapters/thread_adapter.h>

// Note: Currently using built-in thread pool implementation for simplicity and zero dependencies.
// External thread_system integration can be enabled by uncommenting below and implementing
// the integration in the #if EXTERNAL_SYSTEMS_AVAILABLE block.
// #if EXTERNAL_SYSTEMS_AVAILABLE
// #include <kcenon/thread/core/typed_thread_pool.h>
// #endif

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace kcenon::integrated::adapters {

/**
 * @brief Implementation details for thread_adapter
 */
class thread_adapter::impl {
public:
    explicit impl(const thread_config& config)
        : config_(config)
        , initialized_(false)
        , shutdown_(false) {
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
            // External thread_system integration would go here
            // Would provide: priority scheduling, typed pools, job cancellation
            // Current built-in implementation is sufficient for most use cases
#endif

            // Built-in thread pool implementation
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
    }

    std::size_t worker_count() const {
        return workers_.size();
    }

    std::size_t queue_size() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return task_queue_.size();
    }

    void wait_for_completion() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        completion_cv_.wait(lock, [this] {
            return task_queue_.empty() && active_tasks_ == 0;
        });
    }

    bool wait_for_completion_timeout(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return completion_cv_.wait_for(lock, timeout, [this] {
            return task_queue_.empty() && active_tasks_ == 0;
        });
    }

private:
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

    thread_config config_;
    bool initialized_;
    bool shutdown_;

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> task_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable completion_cv_;
    std::size_t active_tasks_ = 0;
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

} // namespace kcenon::integrated::adapters
