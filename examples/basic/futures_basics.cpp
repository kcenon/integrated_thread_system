/**
 * @file futures_basics.cpp
 * @brief Understanding futures and asynchronous results
 * @difficulty Beginner
 * @time 7 minutes
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include "unified_thread_system.h"

using namespace integrated_thread_system;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== Futures Basics Example ===" << std::endl;

    unified_thread_system system;

    // Example 1: Basic future operations
    std::cout << "\n1. Basic future operations:" << std::endl;
    {
        auto future = system.submit([]() {
            std::this_thread::sleep_for(1s);
            return 42;
        });

        // Check if ready (probably not yet)
        if (future.wait_for(0ms) == std::future_status::ready) {
            std::cout << "Result ready immediately!" << std::endl;
        } else {
            std::cout << "Result not ready yet, waiting..." << std::endl;
        }

        // Wait and get result
        int result = future.get();
        std::cout << "Got result: " << result << std::endl;
    }

    // Example 2: Waiting with timeout
    std::cout << "\n2. Waiting with timeout:" << std::endl;
    {
        auto future = system.submit([]() {
            std::this_thread::sleep_for(2s);
            return std::string("Slow operation complete");
        });

        // Try to wait for 500ms
        if (future.wait_for(500ms) == std::future_status::timeout) {
            std::cout << "Still processing after 500ms..." << std::endl;
        }

        // Try again with longer timeout
        if (future.wait_for(2s) == std::future_status::ready) {
            std::cout << "Result: " << future.get() << std::endl;
        }
    }

    // Example 3: Multiple futures with different completion times
    std::cout << "\n3. Multiple futures:" << std::endl;
    {
        auto fast = system.submit([]() {
            std::this_thread::sleep_for(100ms);
            return std::string("Fast task");
        });

        auto medium = system.submit([]() {
            std::this_thread::sleep_for(300ms);
            return std::string("Medium task");
        });

        auto slow = system.submit([]() {
            std::this_thread::sleep_for(500ms);
            return std::string("Slow task");
        });

        // Check which completes first
        while (true) {
            if (fast.wait_for(0ms) == std::future_status::ready) {
                std::cout << "First to complete: " << fast.get() << std::endl;
                break;
            }
            if (medium.wait_for(0ms) == std::future_status::ready) {
                std::cout << "First to complete: " << medium.get() << std::endl;
                break;
            }
            if (slow.wait_for(0ms) == std::future_status::ready) {
                std::cout << "First to complete: " << slow.get() << std::endl;
                break;
            }
            std::this_thread::sleep_for(10ms);
        }

        // Get remaining results
        if (medium.valid()) std::cout << "Second: " << medium.get() << std::endl;
        if (slow.valid()) std::cout << "Third: " << slow.get() << std::endl;
    }

    // Example 4: Fire and forget (not waiting for result)
    std::cout << "\n4. Fire and forget pattern:" << std::endl;
    {
        // Submit task but don't keep the future
        system.submit([]() {
            std::cout << "Background task running..." << std::endl;
            std::this_thread::sleep_for(100ms);
            std::cout << "Background task done!" << std::endl;
        });

        // Continue with other work
        std::cout << "Main thread continues immediately" << std::endl;

        // Give background task time to complete
        std::this_thread::sleep_for(200ms);
    }

    // Example 5: Exception handling with futures
    std::cout << "\n5. Exception handling:" << std::endl;
    {
        auto future = system.submit([]() -> int {
            throw std::runtime_error("Something went wrong!");
            return 42;  // Never reached
        });

        try {
            int result = future.get();
            std::cout << "Result: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Caught exception from task: " << e.what() << std::endl;
        }
    }

    // Example 6: Waiting for all futures
    std::cout << "\n6. Waiting for all tasks:" << std::endl;
    {
        std::vector<std::future<int>> futures;

        // Submit multiple tasks
        for (int i = 0; i < 5; ++i) {
            futures.push_back(system.submit([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
                return i * 10;
            }));
        }

        // Wait for all and collect results
        std::cout << "Results: ";
        for (auto& future : futures) {
            std::cout << future.get() << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "\n=== Futures examples completed! ===" << std::endl;

    return 0;
}

/*
 * What you learned:
 * 1. Checking if a future is ready
 * 2. Waiting with timeouts
 * 3. Handling multiple futures
 * 4. Fire-and-forget patterns
 * 5. Exception propagation through futures
 * 6. Waiting for multiple tasks
 *
 * Next steps:
 * - Move to 02_intermediate/ for priority scheduling
 * - Try error_recovery.cpp for advanced error handling
 */