#!/bin/bash

##############################################################################
# @file generate_docs.sh
# @brief Generate Doxygen documentation for Integrated Thread System
# @author kcenon <kcenon@gmail.com>
# @date 2024
#
# This script generates comprehensive HTML documentation using Doxygen.
# It checks for Doxygen installation, creates necessary directories,
# and opens the generated documentation in the default browser.
##############################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Header
echo "=============================================="
echo "   Integrated Thread System Documentation"
echo "           Generator Script v1.0"
echo "=============================================="
echo ""

# Check for Doxygen installation
print_info "Checking for Doxygen installation..."
if ! command_exists doxygen; then
    print_error "Doxygen is not installed!"
    echo ""
    echo "Please install Doxygen:"
    echo "  - macOS:    brew install doxygen graphviz"
    echo "  - Ubuntu:   sudo apt-get install doxygen graphviz"
    echo "  - Windows:  Download from http://www.doxygen.org"
    echo ""
    exit 1
fi

DOXYGEN_VERSION=$(doxygen --version)
print_success "Doxygen ${DOXYGEN_VERSION} found"

# Check for Graphviz (optional but recommended)
if command_exists dot; then
    DOT_VERSION=$(dot -V 2>&1 | head -n 1)
    print_success "Graphviz found: ${DOT_VERSION}"
else
    print_warning "Graphviz not found. Graphs will not be generated."
    print_info "Install with: brew install graphviz (macOS) or apt-get install graphviz (Linux)"
fi

# Create documentation directory if it doesn't exist
print_info "Creating documentation directory..."
mkdir -p docs/doxygen

# Check if Doxyfile exists
if [ ! -f "Doxyfile" ]; then
    print_error "Doxyfile not found in current directory!"
    print_info "Please run this script from the project root directory."
    exit 1
fi

# Clean previous documentation
if [ -d "docs/doxygen/html" ]; then
    print_info "Cleaning previous documentation..."
    rm -rf docs/doxygen/html
fi

# Generate documentation
print_info "Generating documentation..."
echo ""
doxygen Doxyfile 2>&1 | while IFS= read -r line; do
    if [[ $line == *"Warning"* ]]; then
        echo -e "${YELLOW}${line}${NC}"
    elif [[ $line == *"Error"* ]]; then
        echo -e "${RED}${line}${NC}"
    else
        echo "$line"
    fi
done

# Check if documentation was generated successfully
if [ ! -d "docs/doxygen/html" ]; then
    print_error "Documentation generation failed!"
    exit 1
fi

# Count generated files
HTML_COUNT=$(find docs/doxygen/html -name "*.html" | wc -l)
print_success "Documentation generated successfully!"
print_info "Generated ${HTML_COUNT} HTML files"

# Generate summary
echo ""
echo "=============================================="
echo "           Documentation Summary"
echo "=============================================="
echo "Location:     docs/doxygen/html/index.html"
echo "Main Page:    Based on README.md"
echo "API Docs:     Complete class and function reference"
echo "Examples:     Documented with usage patterns"
echo "Diagrams:     $(if command_exists dot; then echo "Enabled"; else echo "Disabled"; fi)"
echo ""

# Ask if user wants to open documentation
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    print_info "Opening documentation in default browser..."
    open docs/doxygen/html/index.html
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    if command_exists xdg-open; then
        print_info "Opening documentation in default browser..."
        xdg-open docs/doxygen/html/index.html
    else
        print_info "Documentation available at: $(pwd)/docs/doxygen/html/index.html"
    fi
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    # Windows
    print_info "Opening documentation in default browser..."
    start docs/doxygen/html/index.html
else
    print_info "Documentation available at: $(pwd)/docs/doxygen/html/index.html"
fi

print_success "Documentation generation complete!"

# Optional: Generate PDF documentation (requires LaTeX)
if command_exists pdflatex && [ "$1" == "--pdf" ]; then
    print_info "Generating PDF documentation..."
    cd docs/doxygen/latex
    make
    cd ../../..
    print_success "PDF documentation available at: docs/doxygen/latex/refman.pdf"
fi

exit 0