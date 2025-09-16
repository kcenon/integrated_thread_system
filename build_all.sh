#!/bin/bash

# Build all systems with new structure

set -e

echo "Building all systems with new namespace structure..."

SYSTEMS="thread_system logger_system monitoring_system integrated_thread_system"

for system in $SYSTEMS; do
    echo "Building $system..."

    if [ -d "../$system" ]; then
        cd "../$system"

        # Run namespace migration if script exists
        if [ -f "migrate_namespaces.sh" ]; then
            echo "Migrating namespaces for $system..."
            ./migrate_namespaces.sh
        fi

        # Create build directory
        mkdir -p build
        cd build

        # Configure and build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        cmake --build . -j$(nproc)

        echo "$system built successfully!"
    else
        echo "Warning: $system directory not found"
    fi
done

echo "All systems built successfully!"
