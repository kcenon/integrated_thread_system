#!/bin/bash

# Fix include paths for unified_thread_system in all files

echo "Fixing include paths for unified_thread_system.h..."

# Count files to fix
total_files=$(find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | \
    xargs grep -l "unified_thread_system\.h" | \
    grep -v "improved/" | \
    wc -l | tr -d ' ')

echo "Found $total_files files to fix"

# Fix includes in test files
find tests -name "*.cpp" 2>/dev/null | while read file; do
    if grep -q '"unified_thread_system\.h"' "$file"; then
        echo "Fixing: $file"
        sed -i.bak 's|"unified_thread_system\.h"|<kcenon/integrated/unified_thread_system.h>|g' "$file"
    fi
done

# Fix includes in example files (excluding improved directory)
find examples -name "*.cpp" -not -path "*/improved/*" 2>/dev/null | while read file; do
    if grep -q 'unified_thread_system\.h' "$file"; then
        echo "Fixing: $file"
        sed -i.bak 's|"unified_thread_system\.h"|<kcenon/integrated/unified_thread_system.h>|g' "$file"
        sed -i.bak 's|<unified_thread_system\.h>|<kcenon/integrated/unified_thread_system.h>|g' "$file"
    fi
done

# Also fix application_framework includes to use unified_thread_system
find . -name "*.cpp" 2>/dev/null | while read file; do
    if grep -q '"application_framework\.h"' "$file"; then
        echo "Replacing application_framework with unified_thread_system in: $file"
        sed -i.bak 's|"application_framework\.h"|<kcenon/integrated/unified_thread_system.h>|g' "$file"

        # Also need to update the usage patterns
        sed -i.bak 's|application_framework::application_config|unified_thread_system::config|g' "$file"
        sed -i.bak 's|application_framework|unified_thread_system|g' "$file"
    fi
done

# Clean up backup files
find . -name "*.bak" -delete

echo "Include path fixes complete!"

# Verify the changes
echo -e "\nVerifying changes..."
echo "Files still using wrong includes:"
grep -r '"unified_thread_system\.h"' . --include="*.cpp" --include="*.h" | grep -v improved | grep -v ".bak" | wc -l