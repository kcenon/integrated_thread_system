// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// Simple demonstration of task cancellation

#include <kcenon/integrated/unified_thread_system.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace kcenon::integrated;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== Cancellation Demo ===" << std::endl;

    // Create system with default configuration
    unified_config config;
    config.set_thread_count(4);

    unified_thread_system system(config);

    try {
        // Initialize the system
        auto init_result = system.initialize();
        if (init_result.is_err()) {
            std::cerr << "Failed to initialize: " << init_result.error().message << std::endl;
            return 1;
        }

        std::cout << "\n1. Creating cancellation token..." << std::endl;
        auto token = system.create_cancellation_token();

        std::cout << "2. Submitting long-running cancellable task..." << std::endl;

        auto future = system.submit_cancellable(token, []() {
            std::cout << "   Task started, will run for 5 seconds..." << std::endl;

            for (int i = 0; i < 50; ++i) {
                std::this_thread::sleep_for(100ms);

                // In a real scenario, the task should check the token
                // and abort if cancelled
                if (i % 10 == 0) {
                    std::cout << "   Task progress: " << (i * 2) << "%" << std::endl;
                }
            }

            return std::string("Task completed successfully!");
        });

        std::cout << "3. Waiting 1 second..." << std::endl;
        std::this_thread::sleep_for(1s);

        std::cout << "4. Cancelling the task..." << std::endl;
        system.cancel_token(token);

        std::cout << "5. Waiting for task to complete..." << std::endl;

        try {
            // Task should throw exception due to cancellation
            auto result = future.get();
            std::cout << "   Result: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "   Task was cancelled: " << e.what() << std::endl;
        }

        std::cout << "\n=== Testing Non-Cancellable Task ===" << std::endl;

        std::cout << "6. Submitting normal task..." << std::endl;
        auto normal_future = system.submit([]() {
            std::cout << "   Normal task executing..." << std::endl;
            std::this_thread::sleep_for(100ms);
            return 42;
        });

        auto normal_result = normal_future.get();
        std::cout << "   Normal task result: " << normal_result << std::endl;

        std::cout << "\n=== Demo Completed Successfully ===" << std::endl;

        // Shutdown
        system.shutdown();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
