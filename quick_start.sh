#!/bin/bash

# Integrated Thread System Quick Start Script
# One-command setup, build, and test for the unified system

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Display banner
echo -e "${BOLD}${BLUE}================================================${NC}"
echo -e "${BOLD}${BLUE}    Integrated Thread System Quick Start       ${NC}"
echo -e "${BOLD}${BLUE}    Setup → Build → Test → Demo                ${NC}"
echo -e "${BOLD}${BLUE}================================================${NC}"

# Function to print messages
print_status() { echo -e "${BOLD}${BLUE}[STATUS]${NC} $1"; }
print_success() { echo -e "${BOLD}${GREEN}[SUCCESS]${NC} $1"; }
print_error() { echo -e "${BOLD}${RED}[ERROR]${NC} $1"; }
print_warning() { echo -e "${BOLD}${YELLOW}[WARNING]${NC} $1"; }
print_info() { echo -e "${BOLD}${CYAN}[INFO]${NC} $1"; }

# Function to show help
show_help() {
    echo -e "${BOLD}Usage:${NC} $0 [mode]"
    echo ""
    echo -e "${BOLD}Quick Start Modes:${NC}"
    echo "  dev          Developer setup (full build with tests and examples)"
    echo "  demo         Demo mode (minimal build, run examples only)"
    echo "  test         Test mode (build and run comprehensive tests)"
    echo "  benchmark    Benchmark mode (optimized build with performance tests)"
    echo "  minimal      Minimal mode (library only, no extras)"
    echo ""
    echo -e "${BOLD}Options:${NC}"
    echo "  --clean      Force clean setup (remove all previous builds)"
    echo "  --verbose    Show detailed output"
    echo "  --help       Show this help message"
    echo ""
    echo -e "${BOLD}Examples:${NC}"
    echo "  $0           # Default developer setup"
    echo "  $0 demo      # Quick demo"
    echo "  $0 test      # Full test suite"
    echo "  $0 --clean   # Clean setup from scratch"
    echo ""
}

# Function to detect platform and show compatibility
check_compatibility() {
    print_status "Checking system compatibility..."

    local platform=""
    case "$(uname -s)" in
        Darwin) platform="macOS" ;;
        Linux) platform="Linux" ;;
        MINGW*|CYGWIN*|MSYS*) platform="Windows" ;;
        *) platform="Unknown" ;;
    esac

    print_info "Platform: $platform"

    # Check essential tools
    local missing_tools=()
    local recommended_tools=()

    # Essential
    for tool in git cmake; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done

    # At least one compiler
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        missing_tools+=("g++ or clang++")
    fi

    # At least one build system
    if ! command -v make &> /dev/null && ! command -v ninja &> /dev/null; then
        missing_tools+=("make or ninja")
    fi

    # Recommended
    for tool in pkg-config doxygen valgrind; do
        if ! command -v "$tool" &> /dev/null; then
            recommended_tools+=("$tool")
        fi
    done

    if [ ${#missing_tools[@]} -gt 0 ]; then
        print_error "Missing essential tools: ${missing_tools[*]}"
        print_info "Please install these tools and run the script again"
        return 1
    fi

    if [ ${#recommended_tools[@]} -gt 0 ]; then
        print_warning "Missing recommended tools: ${recommended_tools[*]}"
        print_info "Some features may be limited"
    fi

    print_success "System compatibility check passed"
    return 0
}

# Function to setup dependencies
setup_dependencies() {
    print_status "Setting up dependencies..."

    if [ ! -f "dependency.sh" ]; then
        print_error "dependency.sh not found!"
        return 1
    fi

    # Run dependency setup with appropriate options
    local dep_args=()

    if [ $VERBOSE -eq 1 ]; then
        dep_args+=("--verbose")
    fi

    if ! ./dependency.sh "${dep_args[@]}"; then
        print_error "Dependency setup failed"
        return 1
    fi

    print_success "Dependencies setup completed"
    return 0
}

# Function to build the project
build_project() {
    print_status "Building Integrated Thread System..."

    if [ ! -f "build.sh" ]; then
        print_error "build.sh not found!"
        return 1
    fi

    local build_args=()

    # Mode-specific build arguments
    case "$MODE" in
        dev)
            build_args+=("--all" "--debug")
            ;;
        demo)
            build_args+=("--examples" "--release")
            ;;
        test)
            build_args+=("--tests" "--debug" "--sanitizers")
            ;;
        benchmark)
            build_args+=("--benchmark" "--release" "--lto")
            ;;
        minimal)
            build_args+=("--lib-only" "--release")
            ;;
        *)
            build_args+=("--all")
            ;;
    esac

    # Global options
    if [ $CLEAN_BUILD -eq 1 ]; then
        build_args+=("--clean")
    fi

    if [ $VERBOSE -eq 1 ]; then
        build_args+=("--verbose")
    else
        build_args+=("--auto")  # Auto-select compiler to avoid interaction
    fi

    # Run build
    if ! ./build.sh "${build_args[@]}"; then
        print_error "Build failed"
        return 1
    fi

    print_success "Build completed"
    return 0
}

# Function to run tests
run_tests() {
    print_status "Running tests..."

    if [ ! -f "test.sh" ]; then
        print_warning "test.sh not found, skipping tests"
        return 0
    fi

    local test_args=()

    case "$MODE" in
        dev)
            test_args+=("--unit" "--integration")
            ;;
        test)
            test_args+=("--all")
            ;;
        benchmark)
            test_args+=("--performance")
            ;;
        demo|minimal)
            # Skip tests for demo and minimal modes
            print_info "Skipping tests for $MODE mode"
            return 0
            ;;
        *)
            test_args+=("--unit")
            ;;
    esac

    if [ $VERBOSE -eq 1 ]; then
        test_args+=("--verbose")
    fi

    test_args+=("--timeout" "60")  # Shorter timeout for quick start

    if ! ./test.sh "${test_args[@]}"; then
        print_warning "Some tests failed, but continuing..."
        return 0  # Don't fail quick start on test failures
    fi

    print_success "Tests completed"
    return 0
}

# Function to run demo
run_demo() {
    print_status "Running demonstration..."

    # Look for demo executables
    local demo_found=0

    for demo in build/bin/basic_example build/bin/simple_usage_example examples/basic_example; do
        if [ -x "$demo" ]; then
            demo_found=1
            print_info "Running $(basename $demo)..."

            if timeout 30 "$demo"; then
                print_success "Demo $(basename $demo) completed"
            else
                print_warning "Demo $(basename $demo) failed or timed out"
            fi
            break
        fi
    done

    if [ $demo_found -eq 0 ]; then
        print_warning "No demo executables found"
        print_info "Try building with: ./build.sh --examples"
    fi

    return 0
}

# Function to show final status and next steps
show_final_status() {
    echo ""
    echo -e "${BOLD}${GREEN}================================================${NC}"
    echo -e "${BOLD}${GREEN}    Quick Start Completed Successfully         ${NC}"
    echo -e "${BOLD}${GREEN}================================================${NC}"
    echo ""

    print_info "What was accomplished:"
    print_info "  ✓ System compatibility verified"
    print_info "  ✓ Dependencies installed and configured"
    print_info "  ✓ Project built in $MODE mode"

    case "$MODE" in
        dev)
            print_info "  ✓ Development environment ready"
            print_info "  ✓ Tests executed"
            ;;
        demo)
            print_info "  ✓ Demo examples built and run"
            ;;
        test)
            print_info "  ✓ Comprehensive test suite executed"
            ;;
        benchmark)
            print_info "  ✓ Performance benchmarks completed"
            ;;
        minimal)
            print_info "  ✓ Core library built"
            ;;
    esac

    echo ""
    print_info "Available build artifacts:"

    if [ -d "build/bin" ] && [ "$(ls -A build/bin 2>/dev/null)" ]; then
        ls -la build/bin/ | grep -v "^total" | while read -r line; do
            print_info "  • $line"
        done
    else
        print_info "  • Library files in build/"
    fi

    echo ""
    print_info "Next steps:"

    case "$MODE" in
        demo|minimal)
            print_info "  • Run './build.sh --examples' to build more examples"
            print_info "  • Run './test.sh' to execute the test suite"
            ;;
        *)
            print_info "  • Explore examples in the examples/ directory"
            print_info "  • Read documentation in docs/ directory"
            ;;
    esac

    print_info "  • Use './build.sh --help' to see all build options"
    print_info "  • Use './test.sh --help' to see all testing options"

    echo ""
    print_success "🎉 Integrated Thread System is ready to use!"
}

# Parse command line arguments
MODE="dev"
CLEAN_BUILD=0
VERBOSE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        dev|demo|test|benchmark|minimal)
            MODE="$1"
            shift
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Show configuration
print_info "Quick Start Configuration:"
print_info "  Mode: $MODE"
print_info "  Clean build: $([ $CLEAN_BUILD -eq 1 ] && echo 'Yes' || echo 'No')"
print_info "  Verbose: $([ $VERBOSE -eq 1 ] && echo 'Yes' || echo 'No')"
echo ""

# Execute quick start steps
start_time=$(date +%s)

# Step 1: Check compatibility
if ! check_compatibility; then
    print_error "Compatibility check failed"
    exit 1
fi
echo ""

# Step 2: Setup dependencies (only if not in minimal mode)
if [ "$MODE" != "minimal" ] || [ ! -d "../vcpkg" ]; then
    if ! setup_dependencies; then
        print_error "Dependency setup failed"
        exit 1
    fi
    echo ""
fi

# Step 3: Build project
if ! build_project; then
    print_error "Build failed"
    exit 1
fi
echo ""

# Step 4: Run tests (mode dependent)
if [ "$MODE" = "test" ] || [ "$MODE" = "dev" ] || [ "$MODE" = "benchmark" ]; then
    run_tests
    echo ""
fi

# Step 5: Run demo (if demo mode or dev mode)
if [ "$MODE" = "demo" ] || [ "$MODE" = "dev" ]; then
    run_demo
    echo ""
fi

# Calculate elapsed time
end_time=$(date +%s)
elapsed_time=$((end_time - start_time))

print_success "Quick start completed in ${elapsed_time} seconds"

# Show final status and next steps
show_final_status

exit 0