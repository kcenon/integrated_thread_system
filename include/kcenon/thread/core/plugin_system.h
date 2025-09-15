#pragma once

#include <any>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <kcenon/thread/core/event_bus.h>
#include <kcenon/thread/core/configuration_manager.h>
#include <kcenon/thread/interfaces/shared_interfaces.h>

namespace kcenon::thread {

/**
 * @brief Plugin metadata
 */
struct plugin_metadata {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::vector<std::string> dependencies;
    std::unordered_map<std::string, std::string> properties;
};

/**
 * @brief Plugin interface
 */
class plugin_interface {
public:
    virtual ~plugin_interface() = default;

    /**
     * @brief Get plugin metadata
     * @return Plugin metadata
     */
    virtual plugin_metadata get_metadata() const = 0;

    /**
     * @brief Initialize the plugin
     * @param config Configuration manager
     * @param bus Event bus
     * @return True if successful
     */
    virtual bool initialize(configuration_manager& config, event_bus& bus) = 0;

    /**
     * @brief Start the plugin
     * @return True if successful
     */
    virtual bool start() = 0;

    /**
     * @brief Stop the plugin
     */
    virtual void stop() = 0;

    /**
     * @brief Cleanup the plugin
     */
    virtual void cleanup() = 0;

    /**
     * @brief Check if plugin is running
     * @return True if running
     */
    virtual bool is_running() const = 0;

    /**
     * @brief Get provided service
     * @tparam T Service type
     * @return Service instance or nullptr
     */
    template<typename T>
    std::shared_ptr<T> get_service() {
        return std::dynamic_pointer_cast<T>(get_service_impl(typeid(T)));
    }

protected:
    /**
     * @brief Get service implementation
     * @param type Service type info
     * @return Service instance
     */
    virtual std::shared_ptr<void> get_service_impl(const std::type_info& type) {
        return nullptr;
    }
};

/**
 * @brief Plugin load/unload events
 */
struct plugin_loaded_event : event_base {
    std::string plugin_name;
    std::string plugin_version;

    plugin_loaded_event(std::string name, std::string version)
        : plugin_name(std::move(name)), plugin_version(std::move(version)) {}

    std::string type_name() const override {
        return "PluginLoadedEvent";
    }
};

struct plugin_unloaded_event : event_base {
    std::string plugin_name;
    std::string reason;

    plugin_unloaded_event(std::string name, std::string reason_msg = "")
        : plugin_name(std::move(name)), reason(std::move(reason_msg)) {}

    std::string type_name() const override {
        return "PluginUnloadedEvent";
    }
};

/**
 * @brief Plugin manager for dynamic plugin loading
 */
class plugin_manager {
public:
    /**
     * @brief Plugin state
     */
    enum class plugin_state {
        unloaded,
        loaded,
        initialized,
        running,
        stopped,
        error
    };

    /**
     * @brief Plugin info
     */
    struct plugin_info {
        std::unique_ptr<plugin_interface> instance;
        plugin_metadata metadata;
        plugin_state state{plugin_state::unloaded};
        std::filesystem::path path;
        void* handle{nullptr}; // For dynamic library
    };

    /**
     * @brief Constructor
     * @param config Configuration manager
     * @param bus Event bus
     */
    plugin_manager(std::shared_ptr<configuration_manager> config,
                   std::shared_ptr<event_bus> bus)
        : config_manager_(config), event_bus_(bus) {}

    /**
     * @brief Destructor
     */
    ~plugin_manager() {
        unload_all();
    }

    /**
     * @brief Register a plugin instance
     * @param plugin Plugin instance
     * @return True if successful
     */
    bool register_plugin(std::unique_ptr<plugin_interface> plugin) {
        if (!plugin) {
            return false;
        }

        auto metadata = plugin->get_metadata();
        std::lock_guard<std::mutex> lock(mutex_);

        if (plugins_.find(metadata.name) != plugins_.end()) {
            return false; // Already registered
        }

        auto info = std::make_unique<plugin_info>();
        info->instance = std::move(plugin);
        info->metadata = metadata;
        info->state = plugin_state::loaded;

        plugins_[metadata.name] = std::move(info);
        return true;
    }

    /**
     * @brief Load a plugin from file
     * @param plugin_path Path to plugin file
     * @return True if successful
     */
    bool load_plugin(const std::filesystem::path& plugin_path) {
        // This would typically load a dynamic library (.so/.dll)
        // For now, we'll use the register_plugin approach
        // Real implementation would use dlopen/LoadLibrary
        return false; // Not implemented for header-only example
    }

    /**
     * @brief Initialize a plugin
     * @param plugin_name Plugin name
     * @return True if successful
     */
    bool initialize_plugin(const std::string& plugin_name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(plugin_name);
        if (it == plugins_.end()) {
            return false;
        }

        auto& info = it->second;
        if (info->state != plugin_state::loaded) {
            return false;
        }

        // Check dependencies
        for (const auto& dep : info->metadata.dependencies) {
            if (!is_plugin_running(dep)) {
                return false;
            }
        }

        if (info->instance->initialize(*config_manager_, *event_bus_)) {
            info->state = plugin_state::initialized;
            return true;
        }

        info->state = plugin_state::error;
        return false;
    }

    /**
     * @brief Start a plugin
     * @param plugin_name Plugin name
     * @return True if successful
     */
    bool start_plugin(const std::string& plugin_name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(plugin_name);
        if (it == plugins_.end()) {
            return false;
        }

        auto& info = it->second;
        if (info->state != plugin_state::initialized &&
            info->state != plugin_state::stopped) {
            return false;
        }

        if (info->instance->start()) {
            info->state = plugin_state::running;

            // Publish event
            if (event_bus_) {
                event_bus_->publish(plugin_loaded_event(
                    info->metadata.name,
                    info->metadata.version
                ));
            }

            return true;
        }

        info->state = plugin_state::error;
        return false;
    }

    /**
     * @brief Stop a plugin
     * @param plugin_name Plugin name
     * @return True if successful
     */
    bool stop_plugin(const std::string& plugin_name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(plugin_name);
        if (it == plugins_.end()) {
            return false;
        }

        auto& info = it->second;
        if (info->state != plugin_state::running) {
            return false;
        }

        info->instance->stop();
        info->state = plugin_state::stopped;

        // Publish event
        if (event_bus_) {
            event_bus_->publish(plugin_unloaded_event(
                info->metadata.name,
                "Plugin stopped"
            ));
        }

        return true;
    }

    /**
     * @brief Unload a plugin
     * @param plugin_name Plugin name
     * @return True if successful
     */
    bool unload_plugin(const std::string& plugin_name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(plugin_name);
        if (it == plugins_.end()) {
            return false;
        }

        auto& info = it->second;

        // Stop if running
        if (info->state == plugin_state::running) {
            info->instance->stop();
        }

        // Cleanup
        info->instance->cleanup();

        // Remove from registry
        plugins_.erase(it);

        return true;
    }

    /**
     * @brief Unload all plugins
     */
    void unload_all() {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& [name, info] : plugins_) {
            if (info->state == plugin_state::running) {
                info->instance->stop();
            }
            info->instance->cleanup();
        }

        plugins_.clear();
    }

    /**
     * @brief Get plugin info
     * @param plugin_name Plugin name
     * @return Plugin info or nullptr
     */
    const plugin_info* get_plugin_info(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(plugin_name);
        if (it != plugins_.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    /**
     * @brief Check if plugin is loaded
     * @param plugin_name Plugin name
     * @return True if loaded
     */
    bool is_plugin_loaded(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return plugins_.find(plugin_name) != plugins_.end();
    }

    /**
     * @brief Check if plugin is running
     * @param plugin_name Plugin name
     * @return True if running
     */
    bool is_plugin_running(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(plugin_name);
        if (it != plugins_.end()) {
            return it->second->state == plugin_state::running;
        }
        return false;
    }

    /**
     * @brief Get all plugin names
     * @return List of plugin names
     */
    std::vector<std::string> get_plugin_names() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> names;
        for (const auto& [name, info] : plugins_) {
            names.push_back(name);
        }
        return names;
    }

    /**
     * @brief Get service from plugin
     * @tparam T Service type
     * @param plugin_name Plugin name
     * @return Service instance or nullptr
     */
    template<typename T>
    std::shared_ptr<T> get_service(const std::string& plugin_name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(plugin_name);
        if (it != plugins_.end() && it->second->state == plugin_state::running) {
            return it->second->instance->get_service<T>();
        }
        return nullptr;
    }

    /**
     * @brief Get singleton instance
     * @return Plugin manager instance
     */
    static plugin_manager& instance() {
        static plugin_manager instance(
            std::make_shared<configuration_manager>(),
            std::make_shared<event_bus>()
        );
        return instance;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<plugin_info>> plugins_;
    std::shared_ptr<configuration_manager> config_manager_;
    std::shared_ptr<event_bus> event_bus_;
};

} // namespace kcenon::thread