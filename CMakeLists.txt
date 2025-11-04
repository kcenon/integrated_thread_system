cmake_minimum_required(VERSION 3.16)

project(integrated_thread_system
    VERSION 2.0.0
    DESCRIPTION "Enterprise-grade unified threading framework with logging and monitoring"
    LANGUAGES CXX
)

##################################################
# C++ Standard and Build Configuration
##################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

##################################################
# Project Options
##################################################

option(BUILD_WITH_COMMON_SYSTEM "Build with common_system integration (required)" ON)
option(BUILD_TESTS "Build unit and integration tests" ON)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(ENABLE_WEB_DASHBOARD "Enable web dashboard features" ON)
option(USE_SYSTEM_DEPENDENCIES "Use system-installed dependencies instead of vcpkg" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers in debug builds" OFF)
option(INTEGRATED_ENABLE_COVERAGE "Enable code coverage reporting" OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

##################################################
# Dependency Version Requirements
##################################################

set(REQUIRED_COMMON_SYSTEM_VERSION "1.0.0")
set(REQUIRED_THREAD_SYSTEM_VERSION "2.0.0")
set(REQUIRED_LOGGER_SYSTEM_VERSION "1.0.0")
set(REQUIRED_MONITORING_SYSTEM_VERSION "1.0.0")

message(STATUS "Dependency version requirements:")
message(STATUS "  common_system >= ${REQUIRED_COMMON_SYSTEM_VERSION}")
message(STATUS "  thread_system >= ${REQUIRED_THREAD_SYSTEM_VERSION}")
message(STATUS "  logger_system >= ${REQUIRED_LOGGER_SYSTEM_VERSION}")
message(STATUS "  monitoring_system >= ${REQUIRED_MONITORING_SYSTEM_VERSION}")

##################################################
# Compiler-Specific Options
##################################################

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

##################################################
# Dependency Management with FetchContent
##################################################

include(FetchContent)

# Find threading library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

##################################################
# common_system Integration (REQUIRED)
##################################################

if(BUILD_WITH_COMMON_SYSTEM)
    message(STATUS "=== Finding common_system (REQUIRED) ===")

    # Try to find common_system package first
    find_package(common_system CONFIG QUIET)

    if(NOT common_system_FOUND)
        # Check for common_system with prioritized search paths
        set(_INTEGRATED_COMMON_PATHS
            "$ENV{HOME}/Sources/common_system/include"                 # User home directory (cross-platform)
            "${CMAKE_CURRENT_SOURCE_DIR}/../common_system/include"     # Sibling directory (HIGHEST PRIORITY for CI)
            "/usr/local/include/common_system"                         # System installation path
            "${CMAKE_INSTALL_PREFIX}/include/common_system"            # Custom install prefix
            "${CMAKE_CURRENT_SOURCE_DIR}/common_system/include"        # Embedded subfolder (legacy fallback)
        )

        foreach(_path ${_INTEGRATED_COMMON_PATHS})
            if(EXISTS "${_path}/kcenon/common/patterns/result.h")
                message(STATUS "Found common_system at: ${_path}")
                set(COMMON_SYSTEM_INCLUDE_DIR "${_path}")
                set(common_system_FOUND TRUE)
                break()
            endif()
        endforeach()

        if(NOT common_system_FOUND)
            # Fallback to FetchContent
            message(STATUS "Fetching common_system from GitHub...")

            FetchContent_Declare(
                common_system
                GIT_REPOSITORY https://github.com/kcenon/common_system.git
                GIT_TAG        v${REQUIRED_COMMON_SYSTEM_VERSION}
                GIT_SHALLOW    TRUE
                GIT_PROGRESS   TRUE
            )

            FetchContent_MakeAvailable(common_system)

            if(EXISTS "${common_system_SOURCE_DIR}/include/kcenon/common/patterns/result.h")
                message(STATUS "FetchContent: common_system available at ${common_system_SOURCE_DIR}")
                set(COMMON_SYSTEM_INCLUDE_DIR "${common_system_SOURCE_DIR}/include")
                set(common_system_FOUND TRUE)
            else()
                message(FATAL_ERROR "common_system is REQUIRED but not found")
            endif()
        endif()
    else()
        message(STATUS "Found common_system package")
    endif()
else()
    message(FATAL_ERROR "BUILD_WITH_COMMON_SYSTEM is OFF, but common_system is REQUIRED for integrated_thread_system v2.0+")
endif()

##################################################
# External System Dependencies
##################################################

message(STATUS "=== Finding external systems ===")

# Check if we're building as submodule (CI fallback mode)
option(BUILD_INTEGRATED_AS_SUBMODULE "Build without external system dependencies" OFF)

if(BUILD_INTEGRATED_AS_SUBMODULE)
    message(STATUS "Building in submodule mode - using headers-only approach")
    set(EXTERNAL_SYSTEMS_AVAILABLE FALSE)
else()
    # Find thread_system with prioritized search paths
    set(_THREAD_SEARCH_PATHS
        "$ENV{HOME}/Sources/thread_system"                     # User home directory (cross-platform)
        "${CMAKE_CURRENT_SOURCE_DIR}/../thread_system"         # Sibling directory (HIGHEST PRIORITY for CI)
        "/usr/local/lib/thread_system"                         # System installation path
        "${CMAKE_INSTALL_PREFIX}/lib/thread_system"            # Custom install prefix
    )

    set(THREAD_SYSTEM_DIR "")
    foreach(_path ${_THREAD_SEARCH_PATHS})
        if(EXISTS "${_path}")
            message(STATUS "Found thread_system at ${_path}")
            set(THREAD_SYSTEM_DIR "${_path}")
            set(EXTERNAL_SYSTEMS_AVAILABLE TRUE)
            break()
        endif()
    endforeach()

    if(NOT THREAD_SYSTEM_DIR)
        message(WARNING "thread_system not found in any search path")
        set(EXTERNAL_SYSTEMS_AVAILABLE FALSE)
    endif()

    # Find logger_system with prioritized search paths
    if(EXTERNAL_SYSTEMS_AVAILABLE)
        set(_LOGGER_SEARCH_PATHS
            "$ENV{HOME}/Sources/logger_system"                 # User home directory (cross-platform)
            "${CMAKE_CURRENT_SOURCE_DIR}/../logger_system"     # Sibling directory (HIGHEST PRIORITY for CI)
            "/usr/local/lib/logger_system"                     # System installation path
            "${CMAKE_INSTALL_PREFIX}/lib/logger_system"        # Custom install prefix
        )

        set(LOGGER_SYSTEM_DIR "")
        foreach(_path ${_LOGGER_SEARCH_PATHS})
            if(EXISTS "${_path}")
                message(STATUS "Found logger_system at ${_path}")
                set(LOGGER_SYSTEM_DIR "${_path}")
                break()
            endif()
        endforeach()

        if(NOT LOGGER_SYSTEM_DIR)
            message(WARNING "logger_system not found in any search path")
            set(EXTERNAL_SYSTEMS_AVAILABLE FALSE)
        endif()
    endif()

    # Find monitoring_system with prioritized search paths
    if(EXTERNAL_SYSTEMS_AVAILABLE)
        set(_MONITORING_SEARCH_PATHS
            "$ENV{HOME}/Sources/monitoring_system"             # User home directory (cross-platform)
            "${CMAKE_CURRENT_SOURCE_DIR}/../monitoring_system" # Sibling directory (HIGHEST PRIORITY for CI)
            "/usr/local/lib/monitoring_system"                 # System installation path
            "${CMAKE_INSTALL_PREFIX}/lib/monitoring_system"    # Custom install prefix
        )

        set(MONITORING_SYSTEM_DIR "")
        foreach(_path ${_MONITORING_SEARCH_PATHS})
            if(EXISTS "${_path}")
                message(STATUS "Found monitoring_system at ${_path}")
                set(MONITORING_SYSTEM_DIR "${_path}")
                break()
            endif()
        endforeach()

        if(NOT MONITORING_SYSTEM_DIR)
            message(WARNING "monitoring_system not found in any search path")
            set(EXTERNAL_SYSTEMS_AVAILABLE FALSE)
        endif()
    endif()
endif()

if(EXTERNAL_SYSTEMS_AVAILABLE)
    message(STATUS "All external systems found successfully")
else()
    message(WARNING "External systems not complete - some features will be unavailable")
endif()

##################################################
# Optional Dependencies (fmt, nlohmann_json)
##################################################

if(USE_SYSTEM_DEPENDENCIES)
    find_package(fmt REQUIRED)
    find_package(nlohmann_json REQUIRED)
else()
    # Try to find packages first, fall back to FetchContent if not found
    find_package(fmt CONFIG QUIET)
    if(NOT fmt_FOUND)
        message(STATUS "fmt not found, fetching from GitHub...")
        FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 10.1.1
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(fmt)
    endif()

    find_package(nlohmann_json CONFIG QUIET)
    if(NOT nlohmann_json_FOUND)
        message(STATUS "nlohmann_json not found, fetching from GitHub...")
        FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
            GIT_SHALLOW TRUE
        )
        # Set policy to avoid CMake version compatibility issues
        set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
        set(JSON_BuildTests OFF CACHE INTERNAL "")
        set(JSON_Install OFF CACHE INTERNAL "")
        FetchContent_MakeAvailable(nlohmann_json)

        # Store the include directory for header-only usage
        if(TARGET nlohmann_json)
            get_target_property(NLOHMANN_JSON_INCLUDE_DIR nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
            set(nlohmann_json_INCLUDE_DIR "${NLOHMANN_JSON_INCLUDE_DIR}" CACHE INTERNAL "")
        elseif(nlohmann_json_SOURCE_DIR)
            set(nlohmann_json_INCLUDE_DIR "${nlohmann_json_SOURCE_DIR}/include" CACHE INTERNAL "")
        endif()
    endif()
endif()

##################################################
# Source Files Collection
##################################################

set(INTEGRATED_CORE_SOURCES
    src/unified_thread_system.cpp
    src/core/system_coordinator.cpp
    src/core/configuration.cpp
)

set(INTEGRATED_ADAPTER_SOURCES
    src/adapters/thread_adapter.cpp
    src/adapters/logger_adapter.cpp
    src/adapters/monitoring_adapter.cpp
)

set(INTEGRATED_EXTENSION_SOURCES
    src/extensions/distributed_tracing.cpp
    src/extensions/metrics_aggregator.cpp
    src/extensions/plugin_manager.cpp
)

##################################################
# Create Integration Library
##################################################

add_library(integrated_thread_system STATIC
    ${INTEGRATED_CORE_SOURCES}
    ${INTEGRATED_ADAPTER_SOURCES}
    ${INTEGRATED_EXTENSION_SOURCES}
)

# Create enhanced version library (backward compatibility)
add_library(integrated_thread_system_enhanced STATIC
    src/unified_thread_system_enhanced.cpp
)

##################################################
# Target Properties
##################################################

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

##################################################
# Include Directories
##################################################

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

# Add common_system include directories
if(TARGET kcenon::common_system)
    target_link_libraries(integrated_thread_system PUBLIC kcenon::common_system)
    target_link_libraries(integrated_thread_system_enhanced PUBLIC kcenon::common_system)
else()
    target_include_directories(integrated_thread_system
        PUBLIC
            $<BUILD_INTERFACE:${COMMON_SYSTEM_INCLUDE_DIR}>
    )
    target_include_directories(integrated_thread_system_enhanced
        PUBLIC
            $<BUILD_INTERFACE:${COMMON_SYSTEM_INCLUDE_DIR}>
    )
endif()

# Add external system headers if available
if(EXTERNAL_SYSTEMS_AVAILABLE)
    target_include_directories(integrated_thread_system
        PRIVATE
            ${THREAD_SYSTEM_DIR}/include
            ${LOGGER_SYSTEM_DIR}/include
            ${MONITORING_SYSTEM_DIR}/include
    )

    target_include_directories(integrated_thread_system_enhanced
        PRIVATE
            ${THREAD_SYSTEM_DIR}/include
            ${LOGGER_SYSTEM_DIR}/include
            ${MONITORING_SYSTEM_DIR}/include
    )
endif()

# Add nlohmann_json include directory (header-only, no link dependency needed)
if(DEFINED nlohmann_json_INCLUDE_DIR)
    target_include_directories(integrated_thread_system
        PRIVATE ${nlohmann_json_INCLUDE_DIR}
    )
    target_include_directories(integrated_thread_system_enhanced
        PRIVATE ${nlohmann_json_INCLUDE_DIR}
    )
elseif(TARGET nlohmann_json::nlohmann_json)
    target_include_directories(integrated_thread_system
        PRIVATE $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_include_directories(integrated_thread_system_enhanced
        PRIVATE $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()

##################################################
# Link Dependencies
##################################################

# Link external systems if available
if(EXTERNAL_SYSTEMS_AVAILABLE)
    # Try to find thread_system library
    find_library(THREAD_SYSTEM_LIB
        NAMES ThreadSystem libThreadSystem
        PATHS ${THREAD_SYSTEM_DIR}/build/lib
        NO_DEFAULT_PATH
    )

    if(THREAD_SYSTEM_LIB)
        message(STATUS "Found ThreadSystem library: ${THREAD_SYSTEM_LIB}")
    else()
        message(WARNING "ThreadSystem library not found, will link with header-only approach")
    endif()

    # Try to find logger_system library
    find_library(LOGGER_SYSTEM_LIB
        NAMES LoggerSystem libLoggerSystem
        PATHS ${LOGGER_SYSTEM_DIR}/build/lib
        NO_DEFAULT_PATH
    )

    if(LOGGER_SYSTEM_LIB)
        message(STATUS "Found LoggerSystem library: ${LOGGER_SYSTEM_LIB}")
    else()
        message(WARNING "LoggerSystem library not found, will link with header-only approach")
    endif()

    # Try to find monitoring_system library
    find_library(MONITORING_SYSTEM_LIB
        NAMES MonitoringSystem libMonitoringSystem
        PATHS ${MONITORING_SYSTEM_DIR}/build/lib
        NO_DEFAULT_PATH
    )

    if(MONITORING_SYSTEM_LIB)
        message(STATUS "Found MonitoringSystem library: ${MONITORING_SYSTEM_LIB}")
    else()
        message(WARNING "MonitoringSystem library not found, will link with header-only approach")
    endif()
endif()

target_link_libraries(integrated_thread_system
    PUBLIC
        Threads::Threads
    PRIVATE
        fmt::fmt
)

target_link_libraries(integrated_thread_system_enhanced
    PUBLIC
        Threads::Threads
    PRIVATE
        fmt::fmt
)

# Link thread_system if available
if(EXTERNAL_SYSTEMS_AVAILABLE AND THREAD_SYSTEM_LIB)
    target_link_libraries(integrated_thread_system PRIVATE ${THREAD_SYSTEM_LIB})
    target_link_libraries(integrated_thread_system_enhanced PRIVATE ${THREAD_SYSTEM_LIB})
    message(STATUS "Linking ThreadSystem library")
endif()

# Link logger_system if available
if(EXTERNAL_SYSTEMS_AVAILABLE AND LOGGER_SYSTEM_LIB)
    target_link_libraries(integrated_thread_system PRIVATE ${LOGGER_SYSTEM_LIB})
    target_link_libraries(integrated_thread_system_enhanced PRIVATE ${LOGGER_SYSTEM_LIB})
    message(STATUS "Linking LoggerSystem library")
endif()

# Link monitoring_system if available
if(EXTERNAL_SYSTEMS_AVAILABLE AND MONITORING_SYSTEM_LIB)
    target_link_libraries(integrated_thread_system PRIVATE ${MONITORING_SYSTEM_LIB})
    target_link_libraries(integrated_thread_system_enhanced PRIVATE ${MONITORING_SYSTEM_LIB})
    message(STATUS "Linking MonitoringSystem library")
endif()

# Note: nlohmann_json is header-only and included via PRIVATE include directories
# No link dependency needed, avoiding export set issues

# Platform-specific libraries
if(WIN32)
    target_link_libraries(integrated_thread_system PRIVATE ws2_32 wsock32)
    target_link_libraries(integrated_thread_system_enhanced PRIVATE ws2_32 wsock32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(integrated_thread_system PRIVATE dl rt)
    target_link_libraries(integrated_thread_system_enhanced PRIVATE dl rt)
elseif(APPLE)
    target_link_libraries(integrated_thread_system PRIVATE "-framework CoreFoundation")
    target_link_libraries(integrated_thread_system_enhanced PRIVATE "-framework CoreFoundation")
endif()

##################################################
# Compiler Definitions
##################################################

target_compile_definitions(integrated_thread_system
    PUBLIC
        BUILD_WITH_COMMON_SYSTEM
        INTEGRATED_USING_COMMON_INTERFACES
    PRIVATE
        INTEGRATED_THREAD_SYSTEM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        INTEGRATED_THREAD_SYSTEM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        INTEGRATED_THREAD_SYSTEM_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        INTEGRATED_THREAD_SYSTEM_VERSION="${PROJECT_VERSION}"
)

target_compile_definitions(integrated_thread_system_enhanced
    PUBLIC
        BUILD_WITH_COMMON_SYSTEM
        INTEGRATED_USING_COMMON_INTERFACES
    PRIVATE
        INTEGRATED_THREAD_SYSTEM_VERSION="${PROJECT_VERSION}"
)

# Add external systems availability flag
if(EXTERNAL_SYSTEMS_AVAILABLE)
    target_compile_definitions(integrated_thread_system
        PRIVATE EXTERNAL_SYSTEMS_AVAILABLE=1
    )
    target_compile_definitions(integrated_thread_system_enhanced
        PRIVATE EXTERNAL_SYSTEMS_AVAILABLE=1
    )
else()
    target_compile_definitions(integrated_thread_system
        PRIVATE EXTERNAL_SYSTEMS_AVAILABLE=0
    )
    target_compile_definitions(integrated_thread_system_enhanced
        PRIVATE EXTERNAL_SYSTEMS_AVAILABLE=0
    )
endif()

if(ENABLE_WEB_DASHBOARD)
    target_compile_definitions(integrated_thread_system PUBLIC ENABLE_WEB_DASHBOARD=1)
    target_compile_definitions(integrated_thread_system_enhanced PUBLIC ENABLE_WEB_DASHBOARD=1)
endif()

##################################################
# Subdirectories
##################################################

# Add examples
if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")
    add_subdirectory(examples)
endif()

# Add tests
if(BUILD_TESTS)
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
        message(WARNING "GTest not found, skipping tests")
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
        message(WARNING "benchmark not found, skipping benchmarks")
    endif()
endif()

##################################################
# Installation
##################################################

include(GNUInstallDirs)

# Install the library
install(TARGETS integrated_thread_system integrated_thread_system_enhanced
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
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config)
    install(DIRECTORY config/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/config
        FILES_MATCHING PATTERN "*.json"
    )
endif()

# Create and install CMake config files
include(CMakePackageConfigHelpers)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/integrated_thread_system_config_version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Generate config file (if template exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/integrated_thread_system_config.cmake.in")
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/integrated_thread_system_config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/integrated_thread_system_config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/integrated_thread_system
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/integrated_thread_system_config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/integrated_thread_system_config_version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/integrated_thread_system
    )
endif()

# Install version file
install(FILES
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
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY
        )

        add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        )
    endif()
endif()

##################################################
# Configuration Summary
##################################################

message(STATUS "")
message(STATUS "========================================")
message(STATUS "integrated_thread_system v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  BUILD_WITH_COMMON_SYSTEM: ${BUILD_WITH_COMMON_SYSTEM}")
message(STATUS "  BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "  BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "  BUILD_BENCHMARKS: ${BUILD_BENCHMARKS}")
message(STATUS "  ENABLE_WEB_DASHBOARD: ${ENABLE_WEB_DASHBOARD}")
message(STATUS "  ENABLE_SANITIZERS: ${ENABLE_SANITIZERS}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  common_system: ${common_system_FOUND}")
message(STATUS "  thread_system: ${EXTERNAL_SYSTEMS_AVAILABLE}")
message(STATUS "  logger_system: ${EXTERNAL_SYSTEMS_AVAILABLE}")
message(STATUS "  monitoring_system: ${EXTERNAL_SYSTEMS_AVAILABLE}")
message(STATUS "  fmt: ${fmt_FOUND}")
message(STATUS "  nlohmann_json: ${nlohmann_json_FOUND}")
message(STATUS "========================================")
message(STATUS "")
