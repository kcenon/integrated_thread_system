/**
 * @file plugin_system_example.cpp
 * @brief Example demonstrating plugin system usage
 */

#include <iostream>
#include <kcenon/thread/core/plugin_system.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/adapters/thread_pool_executor.h>

using namespace kcenon::thread;

/**
 * @brief Example plugin that provides a calculation service
 */
class calculator_plugin : public plugin_interface {
public:
    plugin_metadata get_metadata() const override {
        return {
            .name = "CalculatorPlugin",
            .version = "1.0.0",
            .author = "kcenon",
            .description = "Simple calculator plugin",
            .dependencies = {},
            .properties = {{"type", "service"}, {"category", "math"}}
        };
    }

    bool initialize(configuration_manager& config, event_bus& bus) override {
        std::cout << "[CalculatorPlugin] Initializing..." << std::endl;
        
        // Subscribe to configuration changes
        config_subscription_ = bus.subscribe<config_changed_event>(
            [this](const config_changed_event& event) {
                std::cout << "[CalculatorPlugin] Config changed: " 
                         << event.config_path << std::endl;
            }
        );
        
        // Load configuration
        precision_ = config.get<int>("calculator.precision", 2);
        
        return true;
    }

    bool start() override {
        std::cout << "[CalculatorPlugin] Starting..." << std::endl;
        is_running_ = true;
        return true;
    }

    void stop() override {
        std::cout << "[CalculatorPlugin] Stopping..." << std::endl;
        is_running_ = false;
    }

    void cleanup() override {
        std::cout << "[CalculatorPlugin] Cleaning up..." << std::endl;
        config_subscription_.unsubscribe();
    }

    bool is_running() const override {
        return is_running_;
    }

    // Calculator service methods
    double add(double a, double b) const {
        return a + b;
    }

    double multiply(double a, double b) const {
        return a * b;
    }

private:
    bool is_running_{false};
    int precision_{2};
    event_bus::subscription config_subscription_;
};

/**
 * @brief Example plugin that uses thread pool service
 */
class worker_plugin : public plugin_interface {
public:
    plugin_metadata get_metadata() const override {
        return {
            .name = "WorkerPlugin",
            .version = "1.0.0",
            .author = "kcenon",
            .description = "Worker plugin using thread pool",
            .dependencies = {},
            .properties = {{"type", "worker"}, {"threads", "4"}}
        };
    }

    bool initialize(configuration_manager& config, event_bus& bus) override {
        std::cout << "[WorkerPlugin] Initializing..." << std::endl;
        
        event_bus_ = &bus;
        
        // Create thread pool
        int num_threads = config.get<int>("worker.threads", 4);
        thread_pool_ = std::make_shared<thread_pool>(num_threads);
        executor_ = std::make_shared<adapters::thread_pool_executor>(thread_pool_);
        
        return true;
    }

    bool start() override {
        std::cout << "[WorkerPlugin] Starting with " 
                 << thread_pool_->get_thread_count() << " threads" << std::endl;
        
        executor_->initialize();
        is_running_ = true;
        
        // Submit sample work
        auto future = executor_->execute([this]() {
            std::cout << "[WorkerPlugin] Processing task..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Publish completion event
            if (event_bus_) {
                performance_alert_event alert(
                    performance_alert_event::severity::info,
                    "Task completed",
                    100.0
                );
                event_bus_->publish(alert);
            }
        });
        
        return true;
    }

    void stop() override {
        std::cout << "[WorkerPlugin] Stopping..." << std::endl;
        executor_->shutdown();
        is_running_ = false;
    }

    void cleanup() override {
        std::cout << "[WorkerPlugin] Cleaning up..." << std::endl;
        thread_pool_.reset();
        executor_.reset();
    }

    bool is_running() const override {
        return is_running_;
    }

protected:
    std::shared_ptr<void> get_service_impl(const std::type_info& type) override {
        if (type == typeid(shared::IExecutor)) {
            return executor_;
        }
        return nullptr;
    }

private:
    bool is_running_{false};
    std::shared_ptr<thread_pool> thread_pool_;
    std::shared_ptr<adapters::thread_pool_executor> executor_;
    event_bus* event_bus_{nullptr};
};

int main() {
    std::cout << "=== Plugin System Example ===" << std::endl;
    
    // Create configuration and event bus
    auto config = std::make_shared<configuration_manager>();
    auto bus = std::make_shared<event_bus>();
    
    // Set some configuration
    config->set("calculator.precision", 3);
    config->set("worker.threads", 2);
    
    // Create plugin manager
    plugin_manager manager(config, bus);
    
    // Subscribe to plugin events
    auto load_sub = bus->subscribe<plugin_loaded_event>(
        [](const plugin_loaded_event& event) {
            std::cout << "[Event] Plugin loaded: " << event.plugin_name 
                     << " v" << event.plugin_version << std::endl;
        }
    );
    
    auto unload_sub = bus->subscribe<plugin_unloaded_event>(
        [](const plugin_unloaded_event& event) {
            std::cout << "[Event] Plugin unloaded: " << event.plugin_name 
                     << " (" << event.reason << ")" << std::endl;
        }
    );
    
    // Register plugins
    std::cout << "\n1. Registering plugins..." << std::endl;
    manager.register_plugin(std::make_unique<calculator_plugin>());
    manager.register_plugin(std::make_unique<worker_plugin>());
    
    // Initialize plugins
    std::cout << "\n2. Initializing plugins..." << std::endl;
    if (!manager.initialize_plugin("CalculatorPlugin")) {
        std::cerr << "Failed to initialize CalculatorPlugin" << std::endl;
    }
    if (!manager.initialize_plugin("WorkerPlugin")) {
        std::cerr << "Failed to initialize WorkerPlugin" << std::endl;
    }
    
    // Start plugins
    std::cout << "\n3. Starting plugins..." << std::endl;
    manager.start_plugin("CalculatorPlugin");
    manager.start_plugin("WorkerPlugin");
    
    // List active plugins
    std::cout << "\n4. Active plugins:" << std::endl;
    for (const auto& name : manager.get_plugin_names()) {
        if (manager.is_plugin_running(name)) {
            const auto* info = manager.get_plugin_info(name);
            std::cout << "   - " << name << " v" << info->metadata.version 
                     << " (" << info->metadata.description << ")" << std::endl;
        }
    }
    
    // Get service from plugin
    std::cout << "\n5. Using plugin services..." << std::endl;
    auto executor_service = manager.get_service<shared::IExecutor>("WorkerPlugin");
    if (executor_service) {
        std::cout << "   Got executor service from WorkerPlugin" << std::endl;
        auto future = executor_service->execute([]() {
            std::cout << "   Executing task via plugin service..." << std::endl;
        });
        future.wait();
    }
    
    // Change configuration
    std::cout << "\n6. Updating configuration..." << std::endl;
    config->set("calculator.precision", 5);
    
    // Wait a bit for async operations
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Stop plugins
    std::cout << "\n7. Stopping plugins..." << std::endl;
    manager.stop_plugin("WorkerPlugin");
    manager.stop_plugin("CalculatorPlugin");
    
    // Unload plugins
    std::cout << "\n8. Unloading plugins..." << std::endl;
    manager.unload_all();
    
    std::cout << "\nExample completed successfully!" << std::endl;
    
    return 0;
}
