/**
 * @file hello_thread.cpp
 * @brief The simplest possible example - Hello World with threads
 * @difficulty Beginner
 * @time 2 minutes
 */

#include <iostream>
#include <kcenon/integrated/unified_thread_system.h>

using namespace kcenon::integrated;

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

/*
 * What you learned:
 * 1. How to create a unified_thread_system
 * 2. How to submit a simple task
 * 3. How to get results using futures
 *
 * Next steps:
 * - Try simple_tasks.cpp for more task examples
 * - Try futures_basics.cpp to learn about futures
 */