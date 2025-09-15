#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <kcenon/thread/core/event_bus.h>
#include <kcenon/thread/core/configuration_manager.h>
#include <kcenon/logger/interfaces/logger_interface.h>

namespace kcenon::integrated {

using namespace thread;

/**
 * @brief Error severity levels
 */
enum class error_severity {
    debug,
    info,
    warning,
    error,
    critical,
    fatal
};

/**
 * @brief Error context information
 */
struct error_context {
    std::string component;      // Component that generated the error
    std::string operation;      // Operation being performed
    std::string thread_id;      // Thread identifier
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> metadata;
    
    error_context(std::string comp = "", std::string op = "")
        : component(std::move(comp))
        , operation(std::move(op))
        , timestamp(std::chrono::system_clock::now()) {
        
        // Capture thread ID
        std::stringstream ss;
        ss << std::this_thread::get_id();
        thread_id = ss.str();
    }
    
    void add_metadata(const std::string& key, const std::string& value) {
        metadata[key] = value;
    }
};

/**
 * @brief Error information
 */
struct error_info {
    error_severity severity;
    std::string message;
    std::string error_code;
    error_context context;
    std::exception_ptr exception;
    std::string stack_trace;
    
    error_info(error_severity sev, std::string msg, std::string code = "")
        : severity(sev)
        , message(std::move(msg))
        , error_code(std::move(code)) {}
};

/**
 * @brief Error occurrence event
 */
struct error_occurred_event : event_base {
    error_info error;
    
    explicit error_occurred_event(error_info err)
        : error(std::move(err)) {}
    
    std::string type_name() const override {
        return "ErrorOccurredEvent";
    }
};

/**
 * @brief Error recovery event
 */
struct error_recovered_event : event_base {
    std::string component;
    std::string error_code;
    std::string recovery_action;
    
    error_recovered_event(std::string comp, std::string code, std::string action)
        : component(std::move(comp))
        , error_code(std::move(code))
        , recovery_action(std::move(action)) {}
    
    std::string type_name() const override {
        return "ErrorRecoveredEvent";
    }
};

/**
 * @brief Error handler strategy interface
 */
class error_handler_strategy {
public:
    virtual ~error_handler_strategy() = default;
    
    /**
     * @brief Handle an error
     * @param error Error information
     * @return True if error was handled
     */
    virtual bool handle(const error_info& error) = 0;
    
    /**
     * @brief Check if this strategy can handle the error
     * @param error Error information
     * @return True if can handle
     */
    virtual bool can_handle(const error_info& error) const = 0;
};

/**
 * @brief Retry strategy for transient errors
 */
class retry_strategy : public error_handler_strategy {
public:
    struct config {
        std::size_t max_retries{3};
        std::chrono::milliseconds initial_delay{100};
        double backoff_multiplier{2.0};
        std::chrono::milliseconds max_delay{10000};
    };
    
    explicit retry_strategy(const config& cfg = {})
        : config_(cfg) {}
    
    bool handle(const error_info& error) override {
        if (!can_handle(error)) {
            return false;
        }
        
        auto it = retry_counts_.find(error.error_code);
        if (it == retry_counts_.end()) {
            retry_counts_[error.error_code] = 0;
        }
        
        if (retry_counts_[error.error_code] >= config_.max_retries) {
            return false; // Max retries reached
        }
        
        // Calculate delay
        auto delay = config_.initial_delay;
        for (std::size_t i = 0; i < retry_counts_[error.error_code]; ++i) {
            delay = std::chrono::milliseconds(
                static_cast<long>(delay.count() * config_.backoff_multiplier)
            );
            if (delay > config_.max_delay) {
                delay = config_.max_delay;
                break;
            }
        }
        
        // Schedule retry
        std::this_thread::sleep_for(delay);
        retry_counts_[error.error_code]++;
        
        return true;
    }
    
    bool can_handle(const error_info& error) const override {
        // Handle transient errors
        return error.severity <= error_severity::warning &&
               (error.error_code.find("TRANSIENT") != std::string::npos ||
                error.error_code.find("TIMEOUT") != std::string::npos ||
                error.error_code.find("RETRY") != std::string::npos);
    }
    
private:
    config config_;
    std::unordered_map<std::string, std::size_t> retry_counts_;
};

/**
 * @brief Circuit breaker strategy for preventing cascading failures
 */
class circuit_breaker_strategy : public error_handler_strategy {
public:
    enum class state {
        closed,     // Normal operation
        open,       // Failing, reject requests
        half_open   // Testing if recovered
    };
    
    struct config {
        std::size_t failure_threshold{5};
        std::chrono::seconds timeout{30};
        std::size_t success_threshold{2};
    };
    
    explicit circuit_breaker_strategy(const config& cfg = {})
        : config_(cfg), state_(state::closed) {}
    
    bool handle(const error_info& error) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        switch (state_) {
            case state::closed:
                failure_count_++;
                if (failure_count_ >= config_.failure_threshold) {
                    state_ = state::open;
                    last_failure_time_ = now;
                    return false; // Circuit opened
                }
                break;
                
            case state::open:
                if (now - last_failure_time_ > config_.timeout) {
                    state_ = state::half_open;
                    success_count_ = 0;
                    failure_count_ = 0;
                    return true; // Try again
                }
                return false; // Still open
                
            case state::half_open:
                if (error.severity >= error_severity::error) {
                    state_ = state::open;
                    last_failure_time_ = now;
                    return false;
                } else {
                    success_count_++;
                    if (success_count_ >= config_.success_threshold) {
                        state_ = state::closed;
                        failure_count_ = 0;
                    }
                    return true;
                }
        }
        
        return true;
    }
    
    bool can_handle(const error_info& error) const override {
        return error.component != ""; // Handle all component errors
    }
    
    state get_state() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }
    
private:
    config config_;
    mutable std::mutex mutex_;
    state state_;
    std::size_t failure_count_{0};
    std::size_t success_count_{0};
    std::chrono::steady_clock::time_point last_failure_time_;
};

/**
 * @brief Centralized error handler
 */
class error_handler {
public:
    /**
     * @brief Error handler configuration
     */
    struct config {
        bool enable_logging{true};
        bool enable_metrics{true};
        bool enable_recovery{true};
        std::size_t max_error_queue_size{1000};
        error_severity min_severity{error_severity::debug};
    };
    
    /**
     * @brief Constructor
     * @param cfg Configuration
     * @param bus Event bus
     */
    explicit error_handler(const config& cfg = {},
                          std::shared_ptr<event_bus> bus = nullptr)
        : config_(cfg), event_bus_(bus) {
        if (!event_bus_) {
            event_bus_ = std::make_shared<event_bus>();
        }
        
        // Register default strategies
        register_strategy("retry", std::make_unique<retry_strategy>());
        register_strategy("circuit_breaker", std::make_unique<circuit_breaker_strategy>());
    }
    
    /**
     * @brief Destructor
     */
    ~error_handler() {
        stop();
    }
    
    /**
     * @brief Report an error
     * @param severity Error severity
     * @param message Error message
     * @param context Error context
     */
    void report_error(error_severity severity,
                      const std::string& message,
                      const error_context& context = {}) {
        error_info error(severity, message);
        error.context = context;
        handle_error(error);
    }
    
    /**
     * @brief Report an exception
     * @param e Exception
     * @param context Error context
     */
    void report_exception(const std::exception& e,
                         const error_context& context = {}) {
        error_info error(error_severity::error, e.what());
        error.context = context;
        error.exception = std::make_exception_ptr(e);
        handle_error(error);
    }
    
    /**
     * @brief Report current exception
     * @param context Error context
     */
    void report_current_exception(const error_context& context = {}) {
        error_info error(error_severity::error, "Unknown exception");
        error.context = context;
        error.exception = std::current_exception();
        
        try {
            std::rethrow_exception(error.exception);
        } catch (const std::exception& e) {
            error.message = e.what();
        } catch (...) {
            error.message = "Unknown exception type";
        }
        
        handle_error(error);
    }
    
    /**
     * @brief Register error handler strategy
     * @param name Strategy name
     * @param strategy Strategy instance
     */
    void register_strategy(const std::string& name,
                          std::unique_ptr<error_handler_strategy> strategy) {
        std::lock_guard<std::mutex> lock(strategies_mutex_);
        strategies_[name] = std::move(strategy);
    }
    
    /**
     * @brief Register error callback
     * @param severity Minimum severity to trigger callback
     * @param callback Callback function
     * @return Callback ID
     */
    std::size_t register_callback(error_severity severity,
                                  std::function<void(const error_info&)> callback) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        auto id = next_callback_id_++;
        callbacks_[id] = {severity, std::move(callback)};
        return id;
    }
    
    /**
     * @brief Unregister callback
     * @param id Callback ID
     */
    void unregister_callback(std::size_t id) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        callbacks_.erase(id);
    }
    
    /**
     * @brief Start error processing
     * @return True if started
     */
    bool start() {
        if (running_.exchange(true)) {
            return false;
        }
        
        processing_thread_ = std::thread(&error_handler::process_errors, this);
        return true;
    }
    
    /**
     * @brief Stop error processing
     */
    void stop() {
        if (running_.exchange(false)) {
            error_cv_.notify_all();
            if (processing_thread_.joinable()) {
                processing_thread_.join();
            }
        }
    }
    
    /**
     * @brief Get error statistics
     * @return Error counts by severity
     */
    std::unordered_map<error_severity, std::size_t> get_statistics() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return error_stats_;
    }
    
    /**
     * @brief Get recent errors
     * @param count Number of errors to retrieve
     * @return Recent errors
     */
    std::vector<error_info> get_recent_errors(std::size_t count = 10) const {
        std::lock_guard<std::mutex> lock(history_mutex_);
        std::vector<error_info> result;
        
        auto it = error_history_.rbegin();
        for (std::size_t i = 0; i < count && it != error_history_.rend(); ++i, ++it) {
            result.push_back(*it);
        }
        
        return result;
    }
    
    /**
     * @brief Get singleton instance
     * @return Error handler instance
     */
    static error_handler& instance() {
        static error_handler instance;
        return instance;
    }
    
private:
    /**
     * @brief Handle an error
     */
    void handle_error(const error_info& error) {
        if (error.severity < config_.min_severity) {
            return;
        }
        
        // Update statistics
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            error_stats_[error.severity]++;
        }
        
        // Add to queue
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (error_queue_.size() >= config_.max_error_queue_size) {
                error_queue_.pop(); // Remove oldest
            }
            error_queue_.push(error);
        }
        error_cv_.notify_one();
        
        // Add to history
        {
            std::lock_guard<std::mutex> lock(history_mutex_);
            if (error_history_.size() >= 100) { // Keep last 100 errors
                error_history_.pop_front();
            }
            error_history_.push_back(error);
        }
        
        // Publish event
        if (event_bus_) {
            event_bus_->publish(error_occurred_event(error));
        }
    }
    
    /**
     * @brief Process errors from queue
     */
    void process_errors() {
        while (running_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            error_cv_.wait(lock, [this] {
                return !error_queue_.empty() || !running_;
            });
            
            if (!running_) break;
            
            auto error = error_queue_.front();
            error_queue_.pop();
            lock.unlock();
            
            // Apply recovery strategies
            if (config_.enable_recovery) {
                apply_recovery_strategies(error);
            }
            
            // Invoke callbacks
            invoke_callbacks(error);
            
            // Log error
            if (config_.enable_logging && logger_) {
                log_error(error);
            }
        }
    }
    
    /**
     * @brief Apply recovery strategies
     */
    void apply_recovery_strategies(const error_info& error) {
        std::lock_guard<std::mutex> lock(strategies_mutex_);
        
        for (const auto& [name, strategy] : strategies_) {
            if (strategy->can_handle(error)) {
                if (strategy->handle(error)) {
                    // Recovery successful
                    if (event_bus_) {
                        event_bus_->publish(error_recovered_event(
                            error.context.component,
                            error.error_code,
                            name
                        ));
                    }
                    break;
                }
            }
        }
    }
    
    /**
     * @brief Invoke error callbacks
     */
    void invoke_callbacks(const error_info& error) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        
        for (const auto& [id, callback_info] : callbacks_) {
            if (error.severity >= callback_info.first) {
                try {
                    callback_info.second(error);
                } catch (...) {
                    // Ignore callback errors
                }
            }
        }
    }
    
    /**
     * @brief Log error
     */
    void log_error(const error_info& error) {
        if (!logger_) return;
        
        std::stringstream ss;
        ss << "[" << error.context.component << "] ";
        ss << error.message;
        
        if (!error.error_code.empty()) {
            ss << " (Code: " << error.error_code << ")";
        }
        
        switch (error.severity) {
            case error_severity::debug:
            case error_severity::info:
                logger_->info(ss.str());
                break;
            case error_severity::warning:
                logger_->warn(ss.str());
                break;
            case error_severity::error:
            case error_severity::critical:
                logger_->error(ss.str());
                break;
            case error_severity::fatal:
                logger_->critical(ss.str());
                break;
        }
    }
    
    config config_;
    std::shared_ptr<event_bus> event_bus_;
    std::shared_ptr<logger::logger_interface> logger_;
    
    std::atomic<bool> running_{false};
    std::thread processing_thread_;
    
    mutable std::mutex queue_mutex_;
    std::queue<error_info> error_queue_;
    std::condition_variable error_cv_;
    
    mutable std::mutex strategies_mutex_;
    std::unordered_map<std::string, std::unique_ptr<error_handler_strategy>> strategies_;
    
    mutable std::mutex callbacks_mutex_;
    std::unordered_map<std::size_t, std::pair<error_severity, std::function<void(const error_info&)>>> callbacks_;
    std::size_t next_callback_id_{1};
    
    mutable std::mutex stats_mutex_;
    std::unordered_map<error_severity, std::size_t> error_stats_;
    
    mutable std::mutex history_mutex_;
    std::deque<error_info> error_history_;
};

} // namespace kcenon::integrated