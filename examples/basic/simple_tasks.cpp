/**
 * @file simple_tasks.cpp
 * @brief Basic task submission patterns
 * @difficulty Beginner
 * @time 5 minutes
 */

#include <iostream>
#include <vector>
#include <numeric>
#include "unified_thread_system.h"

using namespace integrated_thread_system;

int main() {
    std::cout << "=== Simple Tasks Example ===" << std::endl;

    unified_thread_system system;

    // Example 1: Task that returns a value
    std::cout << "\n1. Task with return value:" << std::endl;
    {
        auto future = system.submit([]() {
            int sum = 0;
            for (int i = 1; i <= 100; ++i) {
                sum += i;
            }
            return sum;
        });

        std::cout << "Sum of 1 to 100 = " << future.get() << std::endl;
    }

    // Example 2: Task with parameters (using lambda capture)
    std::cout << "\n2. Task with parameters:" << std::endl;
    {
        int multiplier = 5;
        int value = 10;

        auto future = system.submit([multiplier, value]() {
            return multiplier * value;
        });

        std::cout << multiplier << " * " << value << " = " << future.get() << std::endl;
    }

    // Example 3: Multiple independent tasks
    std::cout << "\n3. Multiple tasks:" << std::endl;
    {
        std::vector<std::future<int>> futures;

        // Submit 5 tasks
        for (int i = 0; i < 5; ++i) {
            futures.push_back(system.submit([i]() {
                return i * i;  // Square the number
            }));
        }

        // Collect results
        std::cout << "Squares: ";
        for (auto& future : futures) {
            std::cout << future.get() << " ";
        }
        std::cout << std::endl;
    }

    // Example 4: Task without return value (void)
    std::cout << "\n4. Task without return value:" << std::endl;
    {
        auto future = system.submit([]() {
            std::cout << "This task just prints a message!" << std::endl;
        });

        future.wait();  // Wait for completion
        std::cout << "Task completed!" << std::endl;
    }

    // Example 5: Task that processes data
    std::cout << "\n5. Data processing task:" << std::endl;
    {
        std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        auto future = system.submit([data]() {
            int sum = std::accumulate(data.begin(), data.end(), 0);
            double average = static_cast<double>(sum) / data.size();
            return average;
        });

        std::cout << "Average of data: " << future.get() << std::endl;
    }

    std::cout << "\n=== All tasks completed successfully! ===" << std::endl;

    return 0;
}

/*
 * What you learned:
 * 1. Tasks can return values of any type
 * 2. Lambda captures pass data to tasks
 * 3. Multiple tasks can run independently
 * 4. Tasks can be void (no return value)
 * 5. Tasks can process complex data
 *
 * Next steps:
 * - Try futures_basics.cpp for advanced future operations
 * - Move to 02_intermediate/priority_jobs.cpp for priority scheduling
 */