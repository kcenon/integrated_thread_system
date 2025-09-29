/**
 * @file hello_thread.cpp
 * @brief The simplest possible example - Hello World with threads
 * @author kcenon <kcenon@gmail.com>
 * @date 2024
 *
 * @details This example demonstrates the most basic usage of the Integrated Thread System.
 * It shows how to create a thread system with zero configuration, submit a simple task,
 * and retrieve the result using futures.
 *
 * @par Difficulty
 * Beginner
 *
 * @par Time to Complete
 * 2 minutes
 *
 * @par Key Concepts
 * - Zero-configuration thread system initialization
 * - Task submission using lambdas
 * - Future-based result retrieval
 *
 * @example
 * @code{.cpp}
 * unified_thread_system system;  // Zero configuration!
 * auto future = system.submit([]() { return 42; });
 * int result = future.get();     // Get the result
 * @endcode
 */

#include <iostream>
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

/**
 * @brief Main function demonstrating basic thread system usage
 * @return 0 on success
 */
int main() {
    std::cout << "=== Hello Thread Example ===" << std::endl;

    // Step 1: Create the thread system with just one line!
    unified_thread_system system;

    // Step 2: Submit a simple task that returns a string
    auto future = system.submit([]() {
        return std::string("Hello from a thread!");
    });

    // Step 3: Get the result
    std::string result = future.get();

    // Step 4: Print the result
    std::cout << "Thread said: " << result << std::endl;

    std::cout << "Success! You've run your first threaded task!" << std::endl;

    return 0;
}

/**
 * @par What You Learned
 * -# How to create a unified_thread_system with zero configuration
 * -# How to submit a simple task using lambda expressions
 * -# How to retrieve results using std::future
 *
 * @par Next Steps
 * - Try @ref simple_tasks.cpp for more task examples
 * - Try @ref futures_basics.cpp to learn about futures in detail
 * - Try @ref priority_jobs.cpp to learn about priority-based execution
 *
 * @see unified_thread_system::submit()
 * @see std::future
 */