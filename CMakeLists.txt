cmake_minimum_required(VERSION 3.16)

project(integrated_thread_system
    VERSION 1.0.0
    DESCRIPTION "Enterprise-grade unified threading framework with logging and monitoring"
    LANGUAGES CXX
)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(BUILD_TESTS "Build unit and integration tests" ON)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(ENABLE_WEB_DASHBOARD "Enable web dashboard features" ON)
option(USE_SYSTEM_DEPENDENCIES "Use system-installed dependencies instead of vcpkg" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers in debug builds" OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
        if(ENABLE_SANITIZERS)
            add_compile_options(-fsanitize=address -fsanitize=undefined)
            add_link_options(-fsanitize=address -fsanitize=undefined)
        endif()
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(/O2)
    endif()
endif()

# Find dependencies
if(USE_SYSTEM_DEPENDENCIES)
    find_package(fmt REQUIRED)
    find_package(nlohmann_json REQUIRED)
else()
    # Use vcpkg
    find_package(fmt CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
endif()

# Find threading library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Find external systems and include their headers
message(STATUS "Finding external systems...")

# Check if we're building as submodule (CI fallback mode)
option(BUILD_INTEGRATED_AS_SUBMODULE "Build without external system dependencies" OFF)

if(BUILD_INTEGRATED_AS_SUBMODULE)
    message(STATUS "Building in submodule mode - using headers-only approach")
    # In submodule mode, we'll use header-only implementations
    set(EXTERNAL_SYSTEMS_AVAILABLE FALSE)
else()
    # Find thread_system
    set(THREAD_SYSTEM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thread_system)
    if(NOT EXISTS ${THREAD_SYSTEM_DIR})
        message(WARNING "thread_system not found. Falling back to headers-only mode.")
        set(EXTERNAL_SYSTEMS_AVAILABLE FALSE)
    else()
        set(EXTERNAL_SYSTEMS_AVAILABLE TRUE)
    endif()

    # Find logger_system
    set(LOGGER_SYSTEM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../logger_system)
    if(NOT EXISTS ${LOGGER_SYSTEM_DIR} AND EXTERNAL_SYSTEMS_AVAILABLE)
        message(WARNING "logger_system not found. Falling back to headers-only mode.")
        set(EXTERNAL_SYSTEMS_AVAILABLE FALSE)
    endif()

    # Find monitoring_system
    set(MONITORING_SYSTEM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../monitoring_system)
    if(NOT EXISTS ${MONITORING_SYSTEM_DIR} AND EXTERNAL_SYSTEMS_AVAILABLE)
        message(WARNING "monitoring_system not found. Falling back to headers-only mode.")
        set(EXTERNAL_SYSTEMS_AVAILABLE FALSE)
    endif()
endif()

if(EXTERNAL_SYSTEMS_AVAILABLE)
    message(STATUS "External systems found successfully")
else()
    message(STATUS "Running in headers-only mode")
endif()

# Create integration library
add_library(integrated_thread_system STATIC
    # Main implementation
    src/unified_thread_system.cpp
)

# Create enhanced version library
add_library(integrated_thread_system_enhanced STATIC
    # Enhanced implementation with advanced features
    src/unified_thread_system_enhanced.cpp
)

# Set target properties
set_target_properties(integrated_thread_system PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME integrated_thread_system
)

set_target_properties(integrated_thread_system_enhanced PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME integrated_thread_system_enhanced
)

# Include directories
target_include_directories(integrated_thread_system
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(integrated_thread_system_enhanced
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add external system headers if available
if(EXTERNAL_SYSTEMS_AVAILABLE)
    target_include_directories(integrated_thread_system
        PRIVATE
            # External system headers
            ${THREAD_SYSTEM_DIR}/include
            ${LOGGER_SYSTEM_DIR}/include
            ${MONITORING_SYSTEM_DIR}/include
    )

    target_include_directories(integrated_thread_system_enhanced
        PRIVATE
            # External system headers
            ${THREAD_SYSTEM_DIR}/include
            ${LOGGER_SYSTEM_DIR}/include
            ${MONITORING_SYSTEM_DIR}/include
    )
endif()

# Link dependencies
target_link_libraries(integrated_thread_system
    PUBLIC
        # System dependencies
        Threads::Threads

    PRIVATE
        # Internal dependencies
        fmt::fmt
        nlohmann_json::nlohmann_json
)

target_link_libraries(integrated_thread_system_enhanced
    PUBLIC
        # System dependencies
        Threads::Threads

    PRIVATE
        # Internal dependencies
        fmt::fmt
        nlohmann_json::nlohmann_json
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(integrated_thread_system PRIVATE ws2_32 wsock32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(integrated_thread_system PRIVATE dl rt)
elseif(APPLE)
    target_link_libraries(integrated_thread_system PRIVATE "-framework CoreFoundation")
endif()

# Compiler definitions
target_compile_definitions(integrated_thread_system
    PRIVATE
        INTEGRATED_THREAD_SYSTEM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        INTEGRATED_THREAD_SYSTEM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        INTEGRATED_THREAD_SYSTEM_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        INTEGRATED_THREAD_SYSTEM_VERSION="${PROJECT_VERSION}"
)

# Add external systems availability flag
if(EXTERNAL_SYSTEMS_AVAILABLE)
    target_compile_definitions(integrated_thread_system
        PRIVATE
            EXTERNAL_SYSTEMS_AVAILABLE=1
    )
else()
    target_compile_definitions(integrated_thread_system
        PRIVATE
            EXTERNAL_SYSTEMS_AVAILABLE=0
    )
endif()

if(ENABLE_WEB_DASHBOARD)
    target_compile_definitions(integrated_thread_system PUBLIC ENABLE_WEB_DASHBOARD=1)
endif()

# Add examples
if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")
    add_subdirectory(examples)
endif()

# Add tests (API alignment in progress)
if(BUILD_TESTS)  # Re-enabling tests after API updates
    message(STATUS "Building tests...")
    enable_testing()

    # Find test dependencies
    if(USE_SYSTEM_DEPENDENCIES)
        find_package(GTest QUIET)
    else()
        find_package(GTest CONFIG QUIET)
    endif()

    if(GTest_FOUND OR GTEST_FOUND)
        message(STATUS "GTest found, adding tests")
        add_subdirectory(tests)
    else()
        message(WARNING "GTest not found, skipping tests. Install GTest or enable the 'testing' feature in vcpkg.json")
    endif()
endif()

# Add benchmarks
if(BUILD_BENCHMARKS)
    message(STATUS "Building benchmarks...")

    # Find benchmark dependencies
    if(USE_SYSTEM_DEPENDENCIES)
        find_package(benchmark QUIET)
    else()
        find_package(benchmark CONFIG QUIET)
    endif()

    if(benchmark_FOUND)
        message(STATUS "benchmark found, adding benchmarks")
        add_subdirectory(benchmarks)
    else()
        message(WARNING "benchmark not found, skipping benchmarks. Install benchmark or enable the 'benchmarking' feature in vcpkg.json")
    endif()
endif()

# Installation
include(GNUInstallDirs)

# Install the library
install(TARGETS integrated_thread_system
    EXPORT integrated_thread_system_targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install configuration files
install(DIRECTORY config/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/config
    FILES_MATCHING PATTERN "*.json"
)

# Create and install CMake config files
include(CMakePackageConfigHelpers)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/integrated_thread_system_config_version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Generate config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/integrated_thread_system_config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/integrated_thread_system_config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/integrated_thread_system
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

# Install CMake config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/integrated_thread_system_config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/integrated_thread_system_config_version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/integrated_thread_system
)

# Install targets
install(EXPORT integrated_thread_system_targets
    FILE integrated_thread_system_targets.cmake
    NAMESPACE integrated_thread_system::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/integrated_thread_system
)

# Create uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "===== Configuration Summary =====")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "  BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "  BUILD_BENCHMARKS: ${BUILD_BENCHMARKS}")
message(STATUS "  ENABLE_WEB_DASHBOARD: ${ENABLE_WEB_DASHBOARD}")
message(STATUS "  USE_SYSTEM_DEPENDENCIES: ${USE_SYSTEM_DEPENDENCIES}")
message(STATUS "  ENABLE_SANITIZERS: ${ENABLE_SANITIZERS}")
message(STATUS "")
message(STATUS "External Systems:")
message(STATUS "  thread_system: ${CMAKE_CURRENT_SOURCE_DIR}/external/thread_system")
message(STATUS "  logger_system: ${CMAKE_CURRENT_SOURCE_DIR}/external/logger_system")
message(STATUS "  monitoring_system: ${CMAKE_CURRENT_SOURCE_DIR}/external/monitoring_system")
message(STATUS "==================================")
message(STATUS "")