// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file health_check_demo.cpp
 * @brief Demonstrates health monitoring features from monitoring_system v4.0.0
 *
 * This example shows:
 * - Registering custom health checks
 * - Automatic health status aggregation
 * - Threshold-based degradation detection
 * - Health check metadata and reporting
 * - Integration with Kubernetes-style liveness/readiness probes
 */

#include <kcenon/integrated/adapters/monitoring_adapter.h>
#include <kcenon/integrated/core/configuration.h>

#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <memory>
#include <thread>

using namespace kcenon::integrated;
using namespace kcenon::integrated::adapters;
using namespace std::chrono_literals;

// Simulated components for health checking
class database_connection {
public:
    bool connect() {
        connected_ = true;
        return true;
    }

    void disconnect() {
        connected_ = false;
    }

    bool is_connected() const {
        return connected_;
    }

    bool ping() const {
        return connected_ && !simulated_failure_;
    }

    void simulate_failure(bool fail) {
        simulated_failure_ = fail;
    }

private:
    std::atomic<bool> connected_{false};
    std::atomic<bool> simulated_failure_{false};
};

class cache_service {
public:
    void start() {
        running_ = true;
    }

    void stop() {
        running_ = false;
    }

    bool is_healthy() const {
        return running_ && hit_rate_ > 0.5;
    }

    void set_hit_rate(double rate) {
        hit_rate_ = rate;
    }

    double get_hit_rate() const {
        return hit_rate_;
    }

private:
    std::atomic<bool> running_{false};
    std::atomic<double> hit_rate_{0.8};
};

class message_queue {
public:
    void connect() {
        connected_ = true;
    }

    void disconnect() {
        connected_ = false;
    }

    bool is_connected() const {
        return connected_;
    }

    size_t get_queue_depth() const {
        return queue_depth_;
    }

    void set_queue_depth(size_t depth) {
        queue_depth_ = depth;
    }

    bool is_healthy() const {
        return connected_ && queue_depth_ < max_queue_depth_;
    }

private:
    std::atomic<bool> connected_{false};
    std::atomic<size_t> queue_depth_{0};
    static constexpr size_t max_queue_depth_ = 10000;
};

void print_health_status(const common::interfaces::health_check_result& health) {
    std::cout << "\n=== Health Check Result ===\n";

    const char* status_str = "UNKNOWN";
    switch (health.status) {
        case common::interfaces::health_status::healthy:
            status_str = "HEALTHY";
            break;
        case common::interfaces::health_status::degraded:
            status_str = "DEGRADED";
            break;
        case common::interfaces::health_status::unhealthy:
            status_str = "UNHEALTHY";
            break;
        default:
            break;
    }

    std::cout << std::format("Status: {}\n", status_str);
    std::cout << std::format("Message: {}\n", health.message);
    std::cout << std::format("Check duration: {}ms\n", health.check_duration.count());

    if (!health.metadata.empty()) {
        std::cout << "Component status:\n";
        for (const auto& [key, value] : health.metadata) {
            std::cout << std::format("  {} = {}\n", key, value);
        }
    }
}

int main() {
    std::cout << "=== Health Check Demo (monitoring_system v4.0.0) ===\n\n";

    // Create simulated components
    database_connection db;
    cache_service cache;
    message_queue mq;

    // Configure monitoring with health checking enabled
    monitoring_config config;
    config.enable_monitoring = true;
    config.enable_health_monitoring = true;
    config.health_check_interval = std::chrono::milliseconds(1000);
    config.cpu_threshold = 80.0;
    config.memory_threshold = 90.0;

    // Create and initialize monitoring adapter
    monitoring_adapter monitor(config);
    auto init_result = monitor.initialize();
    if (!init_result.is_ok()) {
        std::cerr << "Failed to initialize monitoring adapter\n";
        return 1;
    }

    std::cout << "Monitoring adapter initialized.\n";

    // Register health checks for each component
    std::cout << "Registering health checks...\n";

    // Database health check
    monitor.register_health_check("database", [&db]() {
        return db.ping();
    });

    // Cache health check
    monitor.register_health_check("cache", [&cache]() {
        return cache.is_healthy();
    });

    // Message queue health check
    monitor.register_health_check("message_queue", [&mq]() {
        return mq.is_healthy();
    });

    // Custom composite health check
    monitor.register_health_check("critical_path", [&db, &mq]() {
        // Critical path requires both database and message queue
        return db.ping() && mq.is_healthy();
    });

    std::cout << "Health checks registered.\n";

    // Phase 1: All components healthy
    std::cout << "\n--- Phase 1: All Components Starting ---\n";
    db.connect();
    cache.start();
    mq.connect();

    std::this_thread::sleep_for(500ms);

    auto health = monitor.check_health();
    if (health.is_ok()) {
        print_health_status(health.value());
    }

    // Phase 2: Simulate database failure
    std::cout << "\n--- Phase 2: Database Failure ---\n";
    std::cout << "Simulating database connection failure...\n";
    db.simulate_failure(true);

    std::this_thread::sleep_for(500ms);

    health = monitor.check_health();
    if (health.is_ok()) {
        print_health_status(health.value());
    }

    // Phase 3: Database recovers, cache degrades
    std::cout << "\n--- Phase 3: Database Recovers, Cache Degrades ---\n";
    std::cout << "Database recovering, cache hit rate dropping...\n";
    db.simulate_failure(false);
    cache.set_hit_rate(0.3);  // Below 50% threshold

    std::this_thread::sleep_for(500ms);

    health = monitor.check_health();
    if (health.is_ok()) {
        print_health_status(health.value());
    }

    // Phase 4: Message queue backlog
    std::cout << "\n--- Phase 4: Message Queue Backlog ---\n";
    std::cout << "Message queue depth increasing...\n";
    cache.set_hit_rate(0.9);  // Cache recovers
    mq.set_queue_depth(15000);  // Above threshold

    std::this_thread::sleep_for(500ms);

    health = monitor.check_health();
    if (health.is_ok()) {
        print_health_status(health.value());
    }

    // Phase 5: All recovered
    std::cout << "\n--- Phase 5: All Recovered ---\n";
    std::cout << "All components recovering...\n";
    mq.set_queue_depth(100);

    std::this_thread::sleep_for(500ms);

    health = monitor.check_health();
    if (health.is_ok()) {
        print_health_status(health.value());
    }

    // Demonstrate Kubernetes-style probes
    std::cout << "\n=== Kubernetes-Style Probes ===\n";

    // Liveness probe - is the application alive?
    auto liveness_check = [&monitor]() -> bool {
        auto result = monitor.check_health();
        if (!result.is_ok()) return false;
        // Application is "alive" if not completely unhealthy
        return result.value().status != common::interfaces::health_status::unhealthy;
    };

    // Readiness probe - is the application ready to serve traffic?
    auto readiness_check = [&monitor]() -> bool {
        auto result = monitor.check_health();
        if (!result.is_ok()) return false;
        // Application is "ready" only if fully healthy
        return result.value().status == common::interfaces::health_status::healthy;
    };

    std::cout << std::format("Liveness: {}\n", liveness_check() ? "PASS" : "FAIL");
    std::cout << std::format("Readiness: {}\n", readiness_check() ? "PASS" : "FAIL");

    // Get final metrics including health-related ones
    std::cout << "\n=== Health-Related Metrics ===\n";
    auto metrics = monitor.get_metrics();
    if (metrics.is_ok()) {
        for (const auto& m : metrics.value().metrics) {
            if (m.name.find("health") != std::string::npos ||
                m.name.find("threshold") != std::string::npos) {
                std::cout << std::format("{}: {:.2f}\n", m.name, m.value);
            }
        }
    }

    // Shutdown
    std::cout << "\n--- Shutdown ---\n";
    db.disconnect();
    cache.stop();
    mq.disconnect();
    monitor.shutdown();

    std::cout << "Demo completed.\n";
    return 0;
}
