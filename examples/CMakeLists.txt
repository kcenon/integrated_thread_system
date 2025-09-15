# Examples CMakeLists.txt for Integrated Thread System
# This file is included from the main CMakeLists.txt, so no need to find_package

# Helper function to create example executables
function(add_integrated_example name source_file)
    add_executable(${name} ${source_file})
    target_link_libraries(${name}
        PRIVATE
        integrated_thread_system
    )

    # Set C++20 standard
    target_compile_features(${name} PRIVATE cxx_std_20)

    # Add compiler-specific options
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra -pedantic)
    endif()

    # Set output directory
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Add to examples group
    set_property(TARGET ${name} PROPERTY FOLDER "examples")
endfunction()

# Create example executables - start with simple ones
add_integrated_example(hello_thread "01_basic/hello_thread.cpp")
add_integrated_example(simple_tasks "01_basic/simple_tasks.cpp")

# Create a target to build all examples
add_custom_target(examples_all DEPENDS
    hello_thread
    simple_tasks
)

# Install examples if requested
if(INTEGRATED_THREAD_SYSTEM_INSTALL_EXAMPLES)
    install(TARGETS
        basic_usage_example
        comprehensive_example
        typed_thread_pool_demo
        adaptive_optimization_demo
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
        COMPONENT examples
    )

    # Install source files for reference
    install(FILES
        basic_usage_example.cpp
        comprehensive_example.cpp
        typed_thread_pool_demo.cpp
        adaptive_optimization_demo.cpp
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
        COMPONENT examples
    )
endif()

# Add test targets for examples (smoke tests)
if(INTEGRATED_THREAD_SYSTEM_BUILD_TESTS)
    enable_testing()

    # Basic smoke tests - just run examples with timeout
    add_test(NAME basic_usage_example_test
             COMMAND basic_usage_example
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    set_tests_properties(basic_usage_example_test PROPERTIES TIMEOUT 30)

    add_test(NAME typed_thread_pool_demo_test
             COMMAND typed_thread_pool_demo
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    set_tests_properties(typed_thread_pool_demo_test PROPERTIES TIMEOUT 60)

    add_test(NAME adaptive_optimization_demo_test
             COMMAND adaptive_optimization_demo
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    set_tests_properties(adaptive_optimization_demo_test PROPERTIES TIMEOUT 120)

    # Comprehensive example needs more time
    add_test(NAME comprehensive_example_test
             COMMAND comprehensive_example
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    set_tests_properties(comprehensive_example_test PROPERTIES TIMEOUT 180)
endif()

# Documentation for examples
if(INTEGRATED_THREAD_SYSTEM_BUILD_DOCS)
    # Create example documentation
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/README_EXAMPLES.md.in
        ${CMAKE_CURRENT_BINARY_DIR}/README_EXAMPLES.md
        @ONLY
    )
endif()

message(STATUS "Examples configured:")
message(STATUS "  - basic_usage_example: Simple API demonstration")
message(STATUS "  - comprehensive_example: Full feature showcase")
message(STATUS "  - typed_thread_pool_demo: Priority-based scheduling demo")
message(STATUS "  - adaptive_optimization_demo: Queue optimization showcase")