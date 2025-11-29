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
        hello_thread
        simple_tasks
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
        COMPONENT examples
    )

    # Install source files for reference
    install(FILES
        01_basic/hello_thread.cpp
        01_basic/simple_tasks.cpp
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
        COMPONENT examples
    )
endif()

# Add test targets for examples (smoke tests)
if(INTEGRATED_THREAD_SYSTEM_BUILD_TESTS)
    enable_testing()

    # Basic smoke tests - just run examples with timeout
    add_test(NAME hello_thread_test
             COMMAND hello_thread
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    set_tests_properties(hello_thread_test PROPERTIES TIMEOUT 30)

    add_test(NAME simple_tasks_test
             COMMAND simple_tasks
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    set_tests_properties(simple_tasks_test PROPERTIES TIMEOUT 60)
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
message(STATUS "  - hello_thread: Basic threading demonstration")
message(STATUS "  - simple_tasks: Simple task execution showcase")