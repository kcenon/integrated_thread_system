/**
 * @file web_server.cpp
 * @brief Real-world web server implementation using the unified thread system
 * @difficulty Real World
 * @time 30 minutes
 */

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <queue>
#include <regex>
#include <sstream>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

// HTTP types
enum class http_method { GET, POST, PUT, DELETE, HEAD, OPTIONS };
enum class http_status { OK = 200, CREATED = 201, BAD_REQUEST = 400, NOT_FOUND = 404,
                         INTERNAL_ERROR = 500, SERVICE_UNAVAILABLE = 503 };

struct http_request {
    http_method method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::chrono::steady_clock::time_point received_at;
};

struct http_response {
    http_status status;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

class web_server {
private:
    unified_thread_system system_;
    std::atomic<int> active_requests_{0};
    std::atomic<int> total_requests_{0};
    std::atomic<int> errors_{0};

    // Route handlers
    using route_handler = std::function<http_response(const http_request&)>;
    std::unordered_map<std::string, route_handler> routes_;

    // Rate limiting
    struct rate_limit_info {
        std::atomic<int> requests{0};
        std::chrono::steady_clock::time_point window_start;
        std::mutex mutex;
    };
    std::unordered_map<std::string, rate_limit_info> rate_limits_;

    // Cache
    struct cache_entry {
        http_response response;
        std::chrono::steady_clock::time_point expires_at;
    };
    std::unordered_map<std::string, cache_entry> cache_;
    std::shared_mutex cache_mutex_;

public:
    web_server() {
        // Configure for web server workload
        config cfg;
        cfg.enable_all_systems()
           .set_worker_count(std::thread::hardware_concurrency() * 2)  // I/O bound
           .set_queue_capacity(10000)
           .enable_adaptive_optimization(true)
           .set_log_file("webserver.log")
           .enable_async_logging(true)
           .enable_performance_monitoring(true)
           .set_metrics_interval(1s);

        system_ = unified_thread_system(cfg);

        // Register metrics
        system_.register_metric("http_requests_total", metric_type::counter);
        system_.register_metric("http_request_duration_ms", metric_type::gauge);
        system_.register_metric("http_active_requests", metric_type::gauge);
        system_.register_metric("http_errors_total", metric_type::counter);
        system_.register_metric("cache_hits", metric_type::counter);
        system_.register_metric("cache_misses", metric_type::counter);

        // Register health check
        system_.register_health_check("web_server", [this]() {
            bool healthy = active_requests_.load() < 1000;
            std::string message = "Active requests: " + std::to_string(active_requests_.load());
            return health_status{healthy, message};
        });

        // Setup routes
        setup_routes();

        system_.log_info("Web server initialized",
            {{"workers", std::thread::hardware_concurrency() * 2},
             {"max_queue", 10000}});
    }

    void setup_routes() {
        // Health check endpoint
        routes_["/health"] = [this](const http_request& req) {
            auto health = system_.check_health();
            return http_response{
                health.is_healthy ? http_status::OK : http_status::SERVICE_UNAVAILABLE,
                {{"Content-Type", "application/json"}},
                health.is_healthy ? R"({"status":"healthy"})" : R"({"status":"unhealthy"})"
            };
        };

        // API endpoints
        routes_["/api/users"] = [this](const http_request& req) {
            return handle_users_endpoint(req);
        };

        routes_["/api/products"] = [this](const http_request& req) {
            return handle_products_endpoint(req);
        };

        routes_["/api/orders"] = [this](const http_request& req) {
            return handle_orders_endpoint(req);
        };

        // Static content
        routes_["/"] = [this](const http_request& req) {
            return serve_static_content("/index.html");
        };

        // Metrics endpoint
        routes_["/metrics"] = [this](const http_request& req) {
            auto metrics = system_.export_metrics(export_format::prometheus);
            return http_response{
                http_status::OK,
                {{"Content-Type", "text/plain"}},
                metrics
            };
        };
    }

    std::future<http_response> handle_request(const http_request& request) {
        total_requests_.fetch_add(1);
        system_.increment_counter("http_requests_total");

        // Check rate limiting
        if (!check_rate_limit(request)) {
            return system_.submit([this]() {
                system_.log_warning("Rate limit exceeded");
                return http_response{
                    http_status::SERVICE_UNAVAILABLE,
                    {{"Retry-After", "60"}},
                    "Rate limit exceeded"
                };
            });
        }

        // Determine priority based on endpoint
        auto priority = get_request_priority(request);

        // Submit request processing
        return system_.submit_with_priority(priority,
            [this, request]() {
                auto start = std::chrono::steady_clock::now();
                active_requests_.fetch_add(1);
                system_.set_gauge("http_active_requests", active_requests_.load());

                system_.log_info("Processing request",
                    {{"method", to_string(request.method)},
                     {"path", request.path},
                     {"client", request.headers.at("X-Client-IP")}});

                http_response response;

                try {
                    // Check cache for GET requests
                    if (request.method == http_method::GET) {
                        auto cached = get_cached_response(request.path);
                        if (cached.has_value()) {
                            system_.increment_counter("cache_hits");
                            response = cached.value();
                        } else {
                            system_.increment_counter("cache_misses");
                            response = process_request_internal(request);
                            cache_response(request.path, response);
                        }
                    } else {
                        response = process_request_internal(request);
                    }
                } catch (const std::exception& e) {
                    errors_.fetch_add(1);
                    system_.increment_counter("http_errors_total");
                    system_.log_error("Request processing failed",
                        {{"error", e.what()},
                         {"path", request.path}});

                    response = http_response{
                        http_status::INTERNAL_ERROR,
                        {{"Content-Type", "text/plain"}},
                        "Internal server error"
                    };
                }

                active_requests_.fetch_sub(1);
                system_.set_gauge("http_active_requests", active_requests_.load());

                auto duration = std::chrono::steady_clock::now() - start;
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                system_.set_gauge("http_request_duration_ms", ms);

                system_.log_info("Request completed",
                    {{"status", static_cast<int>(response.status)},
                     {"duration_ms", ms},
                     {"path", request.path}});

                return response;
            });
    }

    http_response process_request_internal(const http_request& request) {
        // Find route handler
        auto it = routes_.find(request.path);
        if (it == routes_.end()) {
            // Try pattern matching for dynamic routes
            for (const auto& [pattern, handler] : routes_) {
                if (matches_pattern(request.path, pattern)) {
                    return handler(request);
                }
            }
            return http_response{
                http_status::NOT_FOUND,
                {{"Content-Type", "text/plain"}},
                "Not found"
            };
        }

        return it->second(request);
    }

    http_response handle_users_endpoint(const http_request& req) {
        switch (req.method) {
            case http_method::GET:
                // Simulate database query
                std::this_thread::sleep_for(50ms);
                return http_response{
                    http_status::OK,
                    {{"Content-Type", "application/json"}},
                    R"([{"id":1,"name":"John"},{"id":2,"name":"Jane"}])"
                };

            case http_method::POST:
                // Simulate user creation
                std::this_thread::sleep_for(100ms);
                return http_response{
                    http_status::CREATED,
                    {{"Content-Type", "application/json"}},
                    R"({"id":3,"name":"New User"})"
                };

            default:
                return http_response{
                    http_status::BAD_REQUEST,
                    {{"Content-Type", "text/plain"}},
                    "Method not allowed"
                };
        }
    }

    http_response handle_products_endpoint(const http_request& req) {
        // Simulate product catalog access
        std::this_thread::sleep_for(30ms);
        return http_response{
            http_status::OK,
            {{"Content-Type", "application/json"}},
            R"([{"id":1,"name":"Product A","price":99.99}])"
        };
    }

    http_response handle_orders_endpoint(const http_request& req) {
        if (req.method == http_method::POST) {
            // Process order with transaction
            return system_.submit_critical([this, req]() {
                system_.log_info("Processing order",
                    {{"client", req.headers.at("X-Client-IP")}});

                // Simulate order processing
                std::this_thread::sleep_for(200ms);

                return http_response{
                    http_status::CREATED,
                    {{"Content-Type", "application/json"}},
                    R"({"order_id":"ORD-12345","status":"confirmed"})"
                };
            }).get();
        }

        return http_response{
            http_status::OK,
            {{"Content-Type", "application/json"}},
            R"([])"
        };
    }

    http_response serve_static_content(const std::string& path) {
        // Simulate file reading
        std::this_thread::sleep_for(10ms);
        return http_response{
            http_status::OK,
            {{"Content-Type", "text/html"}},
            "<html><body><h1>Welcome to the Web Server</h1></body></html>"
        };
    }

    job_priority get_request_priority(const http_request& req) {
        // Health checks are critical
        if (req.path == "/health") {
            return job_priority::critical;
        }

        // API endpoints are normal priority
        if (req.path.starts_with("/api/")) {
            // POST/PUT are higher priority than GET
            if (req.method == http_method::POST || req.method == http_method::PUT) {
                return job_priority::normal;
            }
        }

        // Static content and metrics are background
        return job_priority::background;
    }

    bool check_rate_limit(const http_request& req) {
        auto client_ip = req.headers.find("X-Client-IP");
        if (client_ip == req.headers.end()) {
            return true; // No client IP, allow
        }

        auto& limit_info = rate_limits_[client_ip->second];
        std::lock_guard<std::mutex> lock(limit_info.mutex);

        auto now = std::chrono::steady_clock::now();
        if (now - limit_info.window_start > 1min) {
            // Reset window
            limit_info.requests = 0;
            limit_info.window_start = now;
        }

        if (limit_info.requests >= 100) { // 100 requests per minute
            return false;
        }

        limit_info.requests++;
        return true;
    }

    std::optional<http_response> get_cached_response(const std::string& path) {
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        auto it = cache_.find(path);
        if (it != cache_.end()) {
            if (std::chrono::steady_clock::now() < it->second.expires_at) {
                return it->second.response;
            }
        }
        return std::nullopt;
    }

    void cache_response(const std::string& path, const http_response& response) {
        if (response.status == http_status::OK) {
            std::unique_lock<std::shared_mutex> lock(cache_mutex_);
            cache_[path] = {
                response,
                std::chrono::steady_clock::now() + 60s // Cache for 1 minute
            };
        }
    }

    bool matches_pattern(const std::string& path, const std::string& pattern) {
        // Simple pattern matching (in production, use proper routing library)
        return path.find(pattern) == 0;
    }

    std::string to_string(http_method method) {
        switch (method) {
            case http_method::GET: return "GET";
            case http_method::POST: return "POST";
            case http_method::PUT: return "PUT";
            case http_method::DELETE: return "DELETE";
            case http_method::HEAD: return "HEAD";
            case http_method::OPTIONS: return "OPTIONS";
        }
        return "UNKNOWN";
    }

    void run_load_test() {
        std::cout << "=== Web Server Load Test ===" << std::endl;

        // Simulate incoming requests
        std::vector<std::future<http_response>> responses;
        auto test_start = std::chrono::steady_clock::now();

        // Generate load
        for (int i = 0; i < 100; ++i) {
            http_request req;
            req.received_at = std::chrono::steady_clock::now();
            req.headers["X-Client-IP"] = "192.168.1." + std::to_string(i % 10);

            if (i % 10 == 0) {
                // Health check
                req.method = http_method::GET;
                req.path = "/health";
            } else if (i % 5 == 0) {
                // API write
                req.method = http_method::POST;
                req.path = "/api/orders";
                req.body = R"({"product_id":1,"quantity":2})";
            } else {
                // API read
                req.method = http_method::GET;
                req.path = (i % 2 == 0) ? "/api/users" : "/api/products";
            }

            responses.push_back(handle_request(req));
        }

        // Wait for all responses
        int success_count = 0;
        for (auto& future : responses) {
            auto response = future.get();
            if (static_cast<int>(response.status) < 400) {
                success_count++;
            }
        }

        auto test_duration = std::chrono::steady_clock::now() - test_start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(test_duration).count();

        std::cout << "Load test completed:" << std::endl;
        std::cout << "  Total requests: " << responses.size() << std::endl;
        std::cout << "  Successful: " << success_count << std::endl;
        std::cout << "  Failed: " << (responses.size() - success_count) << std::endl;
        std::cout << "  Duration: " << ms << "ms" << std::endl;
        std::cout << "  Throughput: " << (responses.size() * 1000 / ms) << " req/sec" << std::endl;

        // Print metrics
        auto metrics = system_.get_performance_stats();
        std::cout << "\nPerformance metrics:" << std::endl;
        std::cout << "  Average latency: " << metrics.average_latency.count() << "ms" << std::endl;
        std::cout << "  Worker utilization: " << (metrics.worker_utilization * 100) << "%" << std::endl;
        std::cout << "  Queue depth: " << metrics.current_queue_depth << std::endl;
    }
};

int main() {
    try {
        web_server server;
        server.run_load_test();

        std::cout << "\n=== Web Server Features Demonstrated ===" << std::endl;
        std::cout << "✓ Request routing and handling" << std::endl;
        std::cout << "✓ Priority-based request processing" << std::endl;
        std::cout << "✓ Rate limiting per client" << std::endl;
        std::cout << "✓ Response caching" << std::endl;
        std::cout << "✓ Comprehensive logging" << std::endl;
        std::cout << "✓ Metrics and monitoring" << std::endl;
        std::cout << "✓ Health checks" << std::endl;
        std::cout << "✓ Error handling" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}