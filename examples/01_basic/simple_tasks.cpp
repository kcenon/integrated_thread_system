/**
 * @file simple_tasks.cpp
 * @brief Basic task submission patterns for the Integrated Thread System
 * @author kcenon <kcenon@gmail.com>
 * @date 2024
 *
 * @details This example demonstrates various patterns for submitting tasks to the
 * thread system, including tasks with return values, parameter passing through
 * lambda captures, multiple independent tasks, void tasks, and data processing tasks.
 *
 * @par Difficulty
 * Beginner
 *
 * @par Time to Complete
 * 5 minutes
 *
 * @par Key Concepts
 * - Task submission with return values
 * - Lambda capture for parameter passing
 * - Managing multiple futures
 * - Void tasks (no return value)
 * - Data processing in parallel
 */

#include <iostream>
#include <vector>
#include <numeric>
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

/**
 * @brief Main function demonstrating various task submission patterns
 *
 * @details This function shows five different task submission patterns:
 * -# Tasks that return values
 * -# Tasks with parameters via lambda capture
 * -# Multiple independent tasks running in parallel
 * -# Void tasks without return values
 * -# Complex data processing tasks
 *
 * @return 0 on success
 */
int main() {
    std::cout << "=== Simple Tasks Example ===" << std::endl;

    unified_thread_system system;

    /**
     * @section example1 Example 1: Task with Return Value
     * Demonstrates how to submit a task that computes and returns a value.
     */
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

    /**
     * @section example2 Example 2: Task with Parameters
     * Shows how to pass parameters to tasks using lambda captures.
     */
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

    /**
     * @section example3 Example 3: Multiple Independent Tasks
     * Demonstrates parallel execution of multiple tasks and collecting their results.
     */
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

    /**
     * @section example4 Example 4: Void Tasks
     * Shows tasks that perform actions without returning values.
     */
    // Example 4: Task without return value (void)
    std::cout << "\n4. Task without return value:" << std::endl;
    {
        auto future = system.submit([]() {
            std::cout << "This task just prints a message!" << std::endl;
        });

        future.wait();  // Wait for completion
        std::cout << "Task completed!" << std::endl;
    }

    /**
     * @section example5 Example 5: Data Processing Tasks
     * Demonstrates processing complex data structures in parallel.
     */
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

/**
 * @par What You Learned
 * -# Tasks can return values of any type using std::future
 * -# Lambda captures provide a clean way to pass data to tasks
 * -# Multiple tasks can run independently and in parallel
 * -# Tasks can be void (no return value) for side-effect operations
 * -# Tasks can process complex data structures efficiently
 *
 * @par Best Practices
 * - Use value captures for small data, reference captures for large data
 * - Prefer returning values over using shared state
 * - Batch related tasks together for better performance
 *
 * @par Next Steps
 * - Try @ref futures_basics.cpp for advanced future operations
 * - Move to @ref priority_jobs.cpp for priority-based scheduling
 * - Explore @ref web_server.cpp for real-world applications
 *
 * @see unified_thread_system::submit()
 * @see std::future::get()
 * @see std::future::wait()
 */