#!/bin/bash

# Integrated Thread System Build Script
# Unified build script combining patterns from thread_system, logger_system, and monitoring_system

# Color definitions for better readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Display banner
echo -e "${BOLD}${BLUE}============================================${NC}"
echo -e "${BOLD}${BLUE}    Integrated Thread System Build         ${NC}"
echo -e "${BOLD}${BLUE}    Unified Thread/Logger/Monitor          ${NC}"
echo -e "${BOLD}${BLUE}============================================${NC}"

# Display help information
show_help() {
    echo -e "${BOLD}Usage:${NC} $0 [options]"
    echo ""
    echo -e "${BOLD}Build Options:${NC}"
    echo "  --clean           Perform a clean rebuild by removing the build directory"
    echo "  --debug           Build in debug mode (default is release)"
    echo "  --benchmark       Build with benchmarks enabled"
    echo ""
    echo -e "${BOLD}Target Options:${NC}"
    echo "  --all             Build all targets (default)"
    echo "  --lib-only        Build only the unified library"
    echo "  --examples        Build example applications"
    echo "  --tests           Build and run the unit tests"
    echo ""
    echo -e "${BOLD}Integration Options:${NC}"
    echo "  --standalone      Build standalone (without external dependencies)"
    echo "  --with-all        Enable all external system integrations"
    echo "  --with-thread     Enable thread_system integration"
    echo "  --with-logger     Enable logger_system integration"
    echo "  --with-monitor    Enable monitoring_system integration"
    echo ""
    echo -e "${BOLD}C++ Compatibility Options:${NC}"
    echo "  --cpp17           Force C++17 mode (disable C++20 features)"
    echo "  --cpp20           Force C++20 mode (enable all modern features)"
    echo "  --force-fmt       Force fmt library usage over std::format"
    echo "  --no-vcpkg        Skip vcpkg and use system libraries only"
    echo "  --no-jthread      Disable std::jthread even if supported"
    echo "  --no-concepts     Disable concepts even if supported"
    echo ""
    echo -e "${BOLD}Build System Options:${NC}"
    echo "  --ninja           Use Ninja build system (if available)"
    echo "  --make            Force use of Make build system"
    echo "  --cores N         Use N cores for compilation (default: auto-detect)"
    echo "  --verbose         Show detailed build output"
    echo ""
    echo -e "${BOLD}Sanitizer Options:${NC}"
    echo "  --asan            Enable AddressSanitizer"
    echo "  --tsan            Enable ThreadSanitizer"
    echo "  --ubsan           Enable UndefinedBehaviorSanitizer"
    echo "  --msan            Enable MemorySanitizer"
    echo ""
    echo -e "${BOLD}Documentation Options:${NC}"
    echo "  --docs            Generate Doxygen documentation"
    echo "  --clean-docs      Clean and regenerate Doxygen documentation"
    echo ""
    echo -e "${BOLD}Compiler Options:${NC}"
    echo "  --compiler NAME   Use specific compiler (skips interactive selection)"
    echo "  --list-compilers  List available compilers and exit"
    echo "  --auto            Auto-select best available compiler"
    echo "  --select          Interactive compiler selection (default)"
    echo ""
    echo -e "${BOLD}Development Options:${NC}"
    echo "  --lto             Enable Link Time Optimization"
    echo "  --static          Build static libraries"
    echo "  --shared          Build shared libraries (default)"
    echo "  --profiling       Enable profiling information"
    echo ""
    echo -e "${BOLD}Examples:${NC}"
    echo "  $0                                    # Standard build with interactive compiler selection"
    echo "  $0 --clean --debug --tests            # Clean debug build with tests"
    echo "  $0 --cpp17 --force-fmt --asan         # C++17 with AddressSanitizer"
    echo "  $0 --lib-only --cores 16 --verbose    # Library-only build with 16 cores"
    echo "  $0 --standalone --static --lto        # Standalone static build with LTO"
    echo ""
    echo -e "${BOLD}General Options:${NC}"
    echo "  --help            Display this help and exit"
    echo "  --version         Display version information and exit"
    echo ""
}

# Show version information
show_version() {
    echo -e "${BOLD}${CYAN}Integrated Thread System Build Script${NC}"
    echo -e "Version: 1.0.0"
    echo -e "Compatible with: thread_system 2.0+, logger_system 1.5+, monitoring_system 1.0+"
    echo -e "Build date: $(date '+%Y-%m-%d')"
    echo ""
    echo -e "${BOLD}Supported Features:${NC}"
    echo -e "  • C++17/C++20 compatibility"
    echo -e "  • Cross-platform builds (Linux, macOS, Windows)"
    echo -e "  • Multiple compiler support (GCC, Clang, MSVC)"
    echo -e "  • Advanced sanitizers and profiling"
    echo -e "  • Automatic dependency management"
    echo -e "  • Interactive and scripted builds"
}

# Function to print status messages
print_status() {
    echo -e "${BOLD}${BLUE}[STATUS]${NC} $1"
}

# Function to print success messages
print_success() {
    echo -e "${BOLD}${GREEN}[SUCCESS]${NC} $1"
}

# Function to print error messages
print_error() {
    echo -e "${BOLD}${RED}[ERROR]${NC} $1"
}

# Function to print warning messages
print_warning() {
    echo -e "${BOLD}${YELLOW}[WARNING]${NC} $1"
}

# Function to print info messages
print_info() {
    echo -e "${BOLD}${CYAN}[INFO]${NC} $1"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Function to detect platform
detect_platform() {
    case "$(uname -s)" in
        Darwin)
            PLATFORM="macOS"
            PLATFORM_SHORT="darwin"
            ;;
        Linux)
            PLATFORM="Linux"
            PLATFORM_SHORT="linux"
            if [ -f /etc/os-release ]; then
                . /etc/os-release
                PLATFORM_DISTRO="$ID"
            fi
            ;;
        MINGW*|CYGWIN*|MSYS*)
            PLATFORM="Windows"
            PLATFORM_SHORT="windows"
            ;;
        *)
            PLATFORM="Unknown"
            PLATFORM_SHORT="unknown"
            ;;
    esac

    print_info "Platform detected: $PLATFORM"
}

# Function to detect available compilers
detect_compilers() {
    local compilers=()
    local compiler_names=()
    local compiler_types=()
    local compiler_versions=()

    print_status "Scanning for available compilers..."

    # Check for GCC variants
    for version in 14 13 12 11 10 9 8; do
        if command_exists "g++-$version"; then
            local ver=$(g++-$version --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
            compilers+=("g++-$version")
            compiler_names+=("GCC $version")
            compiler_types+=("g++")
            compiler_versions+=("$ver")
        fi
    done

    # Check default GCC
    if command_exists "g++"; then
        local ver=$(g++ --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
        compilers+=("g++")
        compiler_names+=("GCC (system default)")
        compiler_types+=("g++")
        compiler_versions+=("$ver")
    fi

    # Check for Clang variants
    for version in 19 18 17 16 15 14 13 12 11 10; do
        if command_exists "clang++-$version"; then
            local ver=$(clang++-$version --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
            compilers+=("clang++-$version")
            compiler_names+=("Clang $version")
            compiler_types+=("clang++")
            compiler_versions+=("$ver")
        fi
    done

    # Check default Clang
    if command_exists "clang++"; then
        local ver=$(clang++ --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
        compilers+=("clang++")
        compiler_names+=("Clang (system default)")
        compiler_types+=("clang++")
        compiler_versions+=("$ver")
    fi

    # Platform-specific compilers
    if [ "$PLATFORM_SHORT" == "darwin" ]; then
        if command_exists "/usr/bin/clang++"; then
            local ver=$(/usr/bin/clang++ --version 2>/dev/null | head -n1)
            compilers+=("/usr/bin/clang++")
            compiler_names+=("Apple Clang")
            compiler_types+=("clang++")
            compiler_versions+=("Apple")
        fi
    fi

    # Intel compilers
    for intel_compiler in icpc icpx; do
        if command_exists "$intel_compiler"; then
            local ver=$($intel_compiler --version 2>/dev/null | head -n1)
            compilers+=("$intel_compiler")
            compiler_names+=("Intel C++ ($intel_compiler)")
            compiler_types+=("intel")
            compiler_versions+=("Intel")
        fi
    done

    # Export arrays for use in other functions
    DETECTED_COMPILERS=("${compilers[@]}")
    DETECTED_COMPILER_NAMES=("${compiler_names[@]}")
    DETECTED_COMPILER_TYPES=("${compiler_types[@]}")
    DETECTED_COMPILER_VERSIONS=("${compiler_versions[@]}")

    print_info "Found ${#compilers[@]} compiler(s)"
}

# Function to show available compilers
show_compilers() {
    if [ ${#DETECTED_COMPILERS[@]} -eq 0 ]; then
        print_error "No C++ compilers found!"
        print_warning "Please install g++ or clang++ and try again"
        return 1
    fi

    echo ""
    echo -e "${BOLD}${CYAN}Available Compilers:${NC}"
    echo -e "${BOLD}${CYAN}═══════════════════════════════════════════${NC}"

    local cpp20_capable=()
    local cpp17_only=()
    local others=()

    # Categorize compilers by C++20 support
    for i in "${!DETECTED_COMPILERS[@]}"; do
        local compiler="${DETECTED_COMPILERS[$i]}"
        local name="${DETECTED_COMPILER_NAMES[$i]}"
        local type="${DETECTED_COMPILER_TYPES[$i]}"
        local version="${DETECTED_COMPILER_VERSIONS[$i]}"

        # Check C++20 capability (heuristic)
        local cpp20_support="No"
        if [[ "$type" == "g++" ]] && [[ "$version" =~ ^(1[1-9]|[2-9][0-9])\. ]]; then
            cpp20_support="Yes"
            cpp20_capable+=("$i")
        elif [[ "$type" == "clang++" ]] && [[ "$version" =~ ^(1[0-9]|[2-9][0-9])\. ]]; then
            cpp20_support="Yes"
            cpp20_capable+=("$i")
        elif [[ "$version" == "Apple" ]]; then
            cpp20_support="Partial"
            cpp17_only+=("$i")
        elif [[ "$type" == "intel" ]]; then
            cpp20_support="Yes"
            cpp20_capable+=("$i")
        else
            cpp17_only+=("$i")
        fi
    done

    # Display C++20 capable compilers first (recommended)
    if [ ${#cpp20_capable[@]} -gt 0 ]; then
        echo -e "${BOLD}${GREEN}C++20 Capable (Recommended):${NC}"
        local count=1
        for idx in "${cpp20_capable[@]}"; do
            printf "  ${BOLD}%2d)${NC} %-30s ${MAGENTA}[%s]${NC} ${CYAN}v%s${NC}\n" \
                $count "${DETECTED_COMPILER_NAMES[$idx]}" "${DETECTED_COMPILERS[$idx]}" "${DETECTED_COMPILER_VERSIONS[$idx]}"
            count=$((count + 1))
        done
        echo ""
    fi

    # Display C++17 compilers
    if [ ${#cpp17_only[@]} -gt 0 ]; then
        echo -e "${BOLD}${YELLOW}C++17 Compatible:${NC}"
        local count=$((${#cpp20_capable[@]} + 1))
        for idx in "${cpp17_only[@]}"; do
            printf "  ${BOLD}%2d)${NC} %-30s ${MAGENTA}[%s]${NC} ${CYAN}%s${NC}\n" \
                $count "${DETECTED_COMPILER_NAMES[$idx]}" "${DETECTED_COMPILERS[$idx]}" "${DETECTED_COMPILER_VERSIONS[$idx]}"
            count=$((count + 1))
        done
    fi

    echo -e "${BOLD}${CYAN}═══════════════════════════════════════════${NC}"
    return 0
}

# Function to select compiler interactively
select_compiler_interactive() {
    show_compilers
    if [ $? -ne 0 ]; then
        return 1
    fi

    local total_compilers=${#DETECTED_COMPILERS[@]}
    local selected_idx=""

    echo ""
    while true; do
        echo -n -e "${BOLD}Select compiler [1-$total_compilers] (${GREEN}recommended: 1${NC}${BOLD}) or 'q' to quit: ${NC}"
        read -r selected_idx

        if [ "$selected_idx" = "q" ] || [ "$selected_idx" = "Q" ]; then
            print_warning "Build cancelled by user"
            exit 0
        fi

        # Default to first option if empty
        if [ -z "$selected_idx" ]; then
            selected_idx=1
        fi

        # Validate input
        if [[ "$selected_idx" =~ ^[0-9]+$ ]] && [ "$selected_idx" -ge 1 ] && [ "$selected_idx" -le "$total_compilers" ]; then
            break
        else
            print_error "Invalid selection. Please enter a number between 1 and $total_compilers, or 'q' to quit."
        fi
    done

    # Set selected compiler
    local actual_idx=$((selected_idx - 1))
    SELECTED_COMPILER="${DETECTED_COMPILERS[$actual_idx]}"
    SELECTED_COMPILER_NAME="${DETECTED_COMPILER_NAMES[$actual_idx]}"
    SELECTED_COMPILER_TYPE="${DETECTED_COMPILER_TYPES[$actual_idx]}"
    SELECTED_COMPILER_VERSION="${DETECTED_COMPILER_VERSIONS[$actual_idx]}"

    echo ""
    print_success "Selected: ${BOLD}${GREEN}$SELECTED_COMPILER_NAME${NC} (${CYAN}$SELECTED_COMPILER_VERSION${NC})"

    # Determine C and C++ compilers
    setup_compiler_pair
    return 0
}

# Function to setup compiler pair (C and C++)
setup_compiler_pair() {
    case "$SELECTED_COMPILER_TYPE" in
        g++)
            CC_COMPILER="${SELECTED_COMPILER/g++/gcc}"
            CXX_COMPILER="$SELECTED_COMPILER"
            ;;
        clang++)
            CC_COMPILER="${SELECTED_COMPILER/clang++/clang}"
            CXX_COMPILER="$SELECTED_COMPILER"
            ;;
        intel)
            if [[ "$SELECTED_COMPILER" == *"icpc"* ]]; then
                CC_COMPILER="icc"
                CXX_COMPILER="$SELECTED_COMPILER"
            elif [[ "$SELECTED_COMPILER" == *"icpx"* ]]; then
                CC_COMPILER="icx"
                CXX_COMPILER="$SELECTED_COMPILER"
            fi
            ;;
        *)
            CC_COMPILER="$SELECTED_COMPILER"
            CXX_COMPILER="$SELECTED_COMPILER"
            ;;
    esac

    # Verify compilers exist
    if ! command_exists "$CC_COMPILER"; then
        print_warning "C compiler $CC_COMPILER not found, using $SELECTED_COMPILER"
        CC_COMPILER="$SELECTED_COMPILER"
    fi

    if ! command_exists "$CXX_COMPILER"; then
        print_warning "C++ compiler $CXX_COMPILER not found, using $SELECTED_COMPILER"
        CXX_COMPILER="$SELECTED_COMPILER"
    fi

    print_info "C compiler: $CC_COMPILER"
    print_info "C++ compiler: $CXX_COMPILER"
}

# Function to auto-select best compiler
auto_select_compiler() {
    detect_compilers

    if [ ${#DETECTED_COMPILERS[@]} -eq 0 ]; then
        print_error "No compilers found for auto-selection!"
        return 1
    fi

    # Prefer newer GCC or Clang versions
    local best_idx=0
    local best_score=0

    for i in "${!DETECTED_COMPILERS[@]}"; do
        local compiler="${DETECTED_COMPILERS[$i]}"
        local type="${DETECTED_COMPILER_TYPES[$i]}"
        local version="${DETECTED_COMPILER_VERSIONS[$i]}"
        local score=0

        # Scoring system
        if [[ "$type" == "g++" ]]; then
            score=10
            # Bonus for newer versions
            if [[ "$version" =~ ^1[3-9]\. ]]; then score=$((score + 5)); fi
            if [[ "$version" =~ ^[2-9][0-9]\. ]]; then score=$((score + 10)); fi
        elif [[ "$type" == "clang++" ]]; then
            score=12  # Slightly prefer Clang
            if [[ "$version" =~ ^1[5-9]\. ]]; then score=$((score + 5)); fi
            if [[ "$version" =~ ^[2-9][0-9]\. ]]; then score=$((score + 10)); fi
        elif [[ "$type" == "intel" ]]; then
            score=8
        fi

        # Prefer system default compilers
        if [[ "$compiler" == "g++" ]] || [[ "$compiler" == "clang++" ]]; then
            score=$((score + 2))
        fi

        if [ $score -gt $best_score ]; then
            best_score=$score
            best_idx=$i
        fi
    done

    SELECTED_COMPILER="${DETECTED_COMPILERS[$best_idx]}"
    SELECTED_COMPILER_NAME="${DETECTED_COMPILER_NAMES[$best_idx]}"
    SELECTED_COMPILER_TYPE="${DETECTED_COMPILER_TYPES[$best_idx]}"
    SELECTED_COMPILER_VERSION="${DETECTED_COMPILER_VERSIONS[$best_idx]}"

    setup_compiler_pair
    print_success "Auto-selected: ${BOLD}${GREEN}$SELECTED_COMPILER_NAME${NC} (${CYAN}$SELECTED_COMPILER_VERSION${NC})"
    return 0
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking build dependencies..."

    local missing_deps=()
    local optional_deps=()

    # Essential dependencies
    for cmd in cmake git; do
        if ! command_exists "$cmd"; then
            missing_deps+=("$cmd")
        fi
    done

    # Build system (at least one required)
    if ! command_exists "make" && ! command_exists "ninja"; then
        missing_deps+=("make or ninja")
    fi

    # Optional but recommended
    for cmd in pkg-config doxygen; do
        if ! command_exists "$cmd"; then
            optional_deps+=("$cmd")
        fi
    done

    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing required dependencies: ${missing_deps[*]}"
        print_warning "Install missing dependencies and try again"

        # Suggest installation commands based on platform
        case "$PLATFORM_SHORT" in
            linux)
                print_info "Ubuntu/Debian: sudo apt-get install ${missing_deps[*]}"
                print_info "CentOS/RHEL: sudo yum install ${missing_deps[*]}"
                ;;
            darwin)
                print_info "macOS: brew install ${missing_deps[*]}"
                ;;
        esac
        return 1
    fi

    if [ ${#optional_deps[@]} -ne 0 ]; then
        print_warning "Optional dependencies not found: ${optional_deps[*]}"
        print_info "Some features may be disabled"
    fi

    # Check vcpkg if not disabled
    if [ $USE_VCPKG -eq 1 ]; then
        check_vcpkg
    fi

    print_success "All required dependencies satisfied"
    return 0
}

# Function to check vcpkg
check_vcpkg() {
    if [ ! -d "../vcpkg" ]; then
        print_warning "vcpkg not found in parent directory"

        if [ -f "./dependency.sh" ]; then
            print_status "Running dependency setup script..."
            if ! bash ./dependency.sh; then
                print_warning "Failed to setup vcpkg, falling back to system libraries"
                USE_VCPKG=0
                return 0
            fi
        else
            print_warning "No dependency script found, using system libraries"
            USE_VCPKG=0
            return 0
        fi
    fi

    # Test vcpkg functionality
    if [ $USE_VCPKG -eq 1 ]; then
        print_status "Testing vcpkg..."
        if ! ../vcpkg/vcpkg version >/dev/null 2>&1; then
            print_warning "vcpkg not functional, using system libraries"
            USE_VCPKG=0
        else
            print_success "vcpkg ready"
        fi
    fi
}

# Initialize default values
CLEAN_BUILD=0
BUILD_DOCS=0
CLEAN_DOCS=0
BUILD_TYPE="Release"
BUILD_BENCHMARKS=0
TARGET="all"
BUILD_CORES=0
VERBOSE=0
SPECIFIC_COMPILER=""
AUTO_SELECT=0
INTERACTIVE_SELECT=1

# Integration options
STANDALONE_MODE=0
ENABLE_ALL_INTEGRATIONS=0
ENABLE_THREAD_SYSTEM=0
ENABLE_LOGGER_SYSTEM=0
ENABLE_MONITOR_SYSTEM=0

# C++ compatibility options
FORCE_CPP17=0
FORCE_CPP20=0
FORCE_FMT=0
USE_VCPKG=1
DISABLE_JTHREAD=0
DISABLE_CONCEPTS=0

# Build system options
FORCE_NINJA=0
FORCE_MAKE=0

# Sanitizer options
ENABLE_ASAN=0
ENABLE_TSAN=0
ENABLE_UBSAN=0
ENABLE_MSAN=0

# Development options
ENABLE_LTO=0
BUILD_STATIC=0
BUILD_SHARED=1
ENABLE_PROFILING=0

# Process command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --help)
            show_help
            exit 0
            ;;
        --version)
            show_version
            exit 0
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --benchmark)
            BUILD_BENCHMARKS=1
            shift
            ;;
        --all)
            TARGET="all"
            shift
            ;;
        --lib-only)
            TARGET="lib-only"
            shift
            ;;
        --examples)
            TARGET="examples"
            shift
            ;;
        --tests)
            TARGET="tests"
            shift
            ;;
        --standalone)
            STANDALONE_MODE=1
            shift
            ;;
        --with-all)
            ENABLE_ALL_INTEGRATIONS=1
            ENABLE_THREAD_SYSTEM=1
            ENABLE_LOGGER_SYSTEM=1
            ENABLE_MONITOR_SYSTEM=1
            shift
            ;;
        --with-thread)
            ENABLE_THREAD_SYSTEM=1
            shift
            ;;
        --with-logger)
            ENABLE_LOGGER_SYSTEM=1
            shift
            ;;
        --with-monitor)
            ENABLE_MONITOR_SYSTEM=1
            shift
            ;;
        --cpp17)
            FORCE_CPP17=1
            shift
            ;;
        --cpp20)
            FORCE_CPP20=1
            shift
            ;;
        --force-fmt)
            FORCE_FMT=1
            shift
            ;;
        --no-vcpkg)
            USE_VCPKG=0
            shift
            ;;
        --no-jthread)
            DISABLE_JTHREAD=1
            shift
            ;;
        --no-concepts)
            DISABLE_CONCEPTS=1
            shift
            ;;
        --ninja)
            FORCE_NINJA=1
            shift
            ;;
        --make)
            FORCE_MAKE=1
            shift
            ;;
        --cores)
            if [[ $2 =~ ^[0-9]+$ ]]; then
                BUILD_CORES=$2
                shift 2
            else
                print_error "Option --cores requires a numeric argument"
                exit 1
            fi
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        --asan)
            ENABLE_ASAN=1
            shift
            ;;
        --tsan)
            ENABLE_TSAN=1
            shift
            ;;
        --ubsan)
            ENABLE_UBSAN=1
            shift
            ;;
        --msan)
            ENABLE_MSAN=1
            shift
            ;;
        --docs)
            BUILD_DOCS=1
            shift
            ;;
        --clean-docs)
            CLEAN_DOCS=1
            BUILD_DOCS=1
            shift
            ;;
        --compiler)
            if [ -n "$2" ] && [ "${2:0:1}" != "-" ]; then
                SPECIFIC_COMPILER="$2"
                INTERACTIVE_SELECT=0
                shift 2
            else
                print_error "Option --compiler requires an argument"
                exit 1
            fi
            ;;
        --list-compilers)
            detect_platform
            detect_compilers
            show_compilers
            exit 0
            ;;
        --auto)
            AUTO_SELECT=1
            INTERACTIVE_SELECT=0
            shift
            ;;
        --select)
            INTERACTIVE_SELECT=1
            AUTO_SELECT=0
            shift
            ;;
        --lto)
            ENABLE_LTO=1
            shift
            ;;
        --static)
            BUILD_STATIC=1
            BUILD_SHARED=0
            shift
            ;;
        --shared)
            BUILD_SHARED=1
            BUILD_STATIC=0
            shift
            ;;
        --profiling)
            ENABLE_PROFILING=1
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Detect platform
detect_platform

# Set number of cores if not specified
if [ $BUILD_CORES -eq 0 ]; then
    if command_exists nproc; then
        BUILD_CORES=$(nproc)
    elif [ "$PLATFORM_SHORT" == "darwin" ]; then
        BUILD_CORES=$(sysctl -n hw.ncpu)
        # macOS: Use fewer cores to avoid memory issues
        if [ $BUILD_CORES -gt 8 ]; then
            BUILD_CORES=$((BUILD_CORES / 2))
            print_warning "macOS: Reducing cores to $BUILD_CORES to avoid memory pressure"
        fi
    else
        BUILD_CORES=4
    fi
fi

print_info "Using $BUILD_CORES cores for compilation"

# Store original directory
ORIGINAL_DIR=$(pwd)

# Platform-specific settings
if [ "$PLATFORM_SHORT" == "linux" ] && [ "$(uname -m)" == "aarch64" ]; then
    export VCPKG_FORCE_SYSTEM_BINARIES=arm
    print_info "ARM64 detected, setting vcpkg compatibility mode"
fi

# Check dependencies
check_dependencies
if [ $? -ne 0 ]; then
    exit 1
fi

# Compiler selection
detect_compilers

if [ -n "$SPECIFIC_COMPILER" ]; then
    # Use specified compiler
    if command_exists "$SPECIFIC_COMPILER"; then
        SELECTED_COMPILER="$SPECIFIC_COMPILER"
        SELECTED_COMPILER_NAME="$SPECIFIC_COMPILER (specified)"
        CC_COMPILER="$SPECIFIC_COMPILER"
        CXX_COMPILER="$SPECIFIC_COMPILER"
        print_success "Using specified compiler: $SPECIFIC_COMPILER"
    else
        print_error "Specified compiler '$SPECIFIC_COMPILER' not found!"
        exit 1
    fi
elif [ $AUTO_SELECT -eq 1 ]; then
    auto_select_compiler
    if [ $? -ne 0 ]; then
        exit 1
    fi
elif [ $INTERACTIVE_SELECT -eq 1 ]; then
    select_compiler_interactive
    if [ $? -ne 0 ]; then
        exit 1
    fi
else
    # Fallback to auto-select
    auto_select_compiler
    if [ $? -ne 0 ]; then
        exit 1
    fi
fi

# Display build configuration
echo ""
print_info "Build Configuration:"
print_info "  Platform: $PLATFORM"
print_info "  Build Type: $BUILD_TYPE"
print_info "  Target: $TARGET"
print_info "  Compiler: $SELECTED_COMPILER_NAME"
print_info "  Cores: $BUILD_CORES"
print_info "  vcpkg: $([ $USE_VCPKG -eq 1 ] && echo 'Enabled' || echo 'Disabled')"

if [ $STANDALONE_MODE -eq 1 ]; then
    print_info "  Mode: Standalone (no external dependencies)"
else
    print_info "  Thread System: $([ $ENABLE_THREAD_SYSTEM -eq 1 ] && echo 'Enabled' || echo 'Auto-detect')"
    print_info "  Logger System: $([ $ENABLE_LOGGER_SYSTEM -eq 1 ] && echo 'Enabled' || echo 'Auto-detect')"
    print_info "  Monitor System: $([ $ENABLE_MONITOR_SYSTEM -eq 1 ] && echo 'Enabled' || echo 'Auto-detect')"
fi

if [ $FORCE_CPP17 -eq 1 ]; then
    print_info "  C++ Standard: C++17 (forced)"
elif [ $FORCE_CPP20 -eq 1 ]; then
    print_info "  C++ Standard: C++20 (forced)"
else
    print_info "  C++ Standard: Auto-detect"
fi

# Sanitizer info
sanitizers=()
[ $ENABLE_ASAN -eq 1 ] && sanitizers+=("AddressSanitizer")
[ $ENABLE_TSAN -eq 1 ] && sanitizers+=("ThreadSanitizer")
[ $ENABLE_UBSAN -eq 1 ] && sanitizers+=("UBSanitizer")
[ $ENABLE_MSAN -eq 1 ] && sanitizers+=("MemorySanitizer")

if [ ${#sanitizers[@]} -gt 0 ]; then
    print_info "  Sanitizers: ${sanitizers[*]}"
fi

echo ""

# Clean build if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    print_status "Performing clean build..."
    rm -rf build
    print_success "Build directory cleaned"
fi

# Create build directory
if [ ! -d "build" ]; then
    print_status "Creating build directory..."
    mkdir -p build
fi

# Enter build directory
cd build || { print_error "Failed to enter build directory"; exit 1; }

# Prepare CMake arguments
CMAKE_ARGS=()

# Basic configuration
CMAKE_ARGS+=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")
CMAKE_ARGS+=("-DCMAKE_C_COMPILER=$CC_COMPILER")
CMAKE_ARGS+=("-DCMAKE_CXX_COMPILER=$CXX_COMPILER")

# vcpkg configuration
if [ $USE_VCPKG -eq 1 ]; then
    CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake")
    print_status "Using vcpkg for dependency management"
else
    print_status "Using system libraries (vcpkg disabled)"
fi

# C++ standard configuration
if [ $FORCE_CPP17 -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_FORCE_CPP17=ON")
elif [ $FORCE_CPP20 -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_FORCE_CPP20=ON")
fi

# Format library configuration
if [ $FORCE_FMT -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_FORCE_FMT=ON")
fi

# Feature disable flags
if [ $DISABLE_JTHREAD -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_DISABLE_JTHREAD=ON")
fi

if [ $DISABLE_CONCEPTS -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_DISABLE_CONCEPTS=ON")
fi

# Integration configuration
if [ $STANDALONE_MODE -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_STANDALONE=ON")
else
    [ $ENABLE_THREAD_SYSTEM -eq 1 ] && CMAKE_ARGS+=("-DINTEGRATED_WITH_THREAD_SYSTEM=ON")
    [ $ENABLE_LOGGER_SYSTEM -eq 1 ] && CMAKE_ARGS+=("-DINTEGRATED_WITH_LOGGER_SYSTEM=ON")
    [ $ENABLE_MONITOR_SYSTEM -eq 1 ] && CMAKE_ARGS+=("-DINTEGRATED_WITH_MONITOR_SYSTEM=ON")
fi

# Target configuration
case $TARGET in
    lib-only)
        CMAKE_ARGS+=("-DINTEGRATED_BUILD_EXAMPLES=OFF")
        CMAKE_ARGS+=("-DINTEGRATED_BUILD_TESTS=OFF")
        ;;
    examples)
        CMAKE_ARGS+=("-DINTEGRATED_BUILD_EXAMPLES=ON")
        CMAKE_ARGS+=("-DINTEGRATED_BUILD_TESTS=OFF")
        ;;
    tests)
        CMAKE_ARGS+=("-DINTEGRATED_BUILD_TESTS=ON")
        ;;
esac

# Benchmark configuration
if [ $BUILD_BENCHMARKS -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_BUILD_BENCHMARKS=ON")
fi

# Sanitizer configuration
if [ $ENABLE_ASAN -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_ENABLE_ASAN=ON")
fi

if [ $ENABLE_TSAN -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_ENABLE_TSAN=ON")
fi

if [ $ENABLE_UBSAN -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_ENABLE_UBSAN=ON")
fi

if [ $ENABLE_MSAN -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_ENABLE_MSAN=ON")
fi

# Development options
if [ $ENABLE_LTO -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_ENABLE_LTO=ON")
fi

if [ $BUILD_STATIC -eq 1 ]; then
    CMAKE_ARGS+=("-DBUILD_SHARED_LIBS=OFF")
else
    CMAKE_ARGS+=("-DBUILD_SHARED_LIBS=ON")
fi

if [ $ENABLE_PROFILING -eq 1 ]; then
    CMAKE_ARGS+=("-DINTEGRATED_ENABLE_PROFILING=ON")
fi

# Build system preference
if [ $FORCE_NINJA -eq 1 ] && command_exists ninja; then
    CMAKE_ARGS+=("-G" "Ninja")
elif [ $FORCE_MAKE -eq 1 ]; then
    CMAKE_ARGS+=("-G" "Unix Makefiles")
fi

# Run CMake configuration
print_status "Configuring project with CMake..."

if [ $VERBOSE -eq 1 ]; then
    print_info "CMake command: cmake .. ${CMAKE_ARGS[*]}"
fi

cmake .. "${CMAKE_ARGS[@]}"

# Check CMake result
if [ $? -ne 0 ]; then
    print_error "CMake configuration failed"

    if [ $USE_VCPKG -eq 1 ]; then
        print_warning "Retrying without vcpkg..."
        USE_VCPKG=0

        # Remove vcpkg-related arguments
        NEW_CMAKE_ARGS=()
        for arg in "${CMAKE_ARGS[@]}"; do
            if [[ "$arg" != "-DCMAKE_TOOLCHAIN_FILE="* ]]; then
                NEW_CMAKE_ARGS+=("$arg")
            fi
        done

        # Clean and retry
        rm -rf CMakeCache.txt CMakeFiles/
        cmake .. "${NEW_CMAKE_ARGS[@]}"

        if [ $? -ne 0 ]; then
            print_error "CMake configuration failed even without vcpkg"
            cd "$ORIGINAL_DIR"
            exit 1
        else
            print_success "CMake configuration succeeded without vcpkg"
        fi
    else
        cd "$ORIGINAL_DIR"
        exit 1
    fi
else
    print_success "CMake configuration succeeded"
fi

# Detect build system
BUILD_SYSTEM=""
if [ -f "build.ninja" ]; then
    BUILD_SYSTEM="ninja"
    BUILD_CMD="ninja"
    BUILD_ARGS=()
    [ $VERBOSE -eq 1 ] && BUILD_ARGS+=("-v")
elif [ -f "Makefile" ]; then
    BUILD_SYSTEM="make"
    BUILD_CMD="make"
    BUILD_ARGS=("-j$BUILD_CORES")
    [ $VERBOSE -eq 1 ] && BUILD_ARGS+=("VERBOSE=1")
else
    print_error "No build system detected"
    cd "$ORIGINAL_DIR"
    exit 1
fi

print_info "Using build system: $BUILD_SYSTEM"

# Build the project
print_status "Building project..."

if [ "$BUILD_SYSTEM" == "ninja" ]; then
    $BUILD_CMD -j"$BUILD_CORES" "${BUILD_ARGS[@]}"
else
    $BUILD_CMD "${BUILD_ARGS[@]}"
fi

# Check build result
if [ $? -ne 0 ]; then
    print_error "Build failed"
    cd "$ORIGINAL_DIR"
    exit 1
fi

print_success "Build completed successfully!"

# Run tests if requested
if [ "$TARGET" == "tests" ]; then
    print_status "Running tests..."

    # Use ctest if available
    if command_exists ctest; then
        if [ $VERBOSE -eq 1 ]; then
            ctest --verbose --output-on-failure
        else
            ctest --output-on-failure
        fi

        if [ $? -eq 0 ]; then
            print_success "All tests passed!"
        else
            print_error "Some tests failed"
        fi
    else
        # Try running test executables directly
        local test_count=0
        local test_failed=0

        for test_exe in bin/*test* tests/*test*; do
            if [ -x "$test_exe" ]; then
                test_count=$((test_count + 1))
                print_status "Running $(basename "$test_exe")..."

                if "$test_exe"; then
                    print_success "$(basename "$test_exe") passed"
                else
                    print_error "$(basename "$test_exe") failed"
                    test_failed=1
                fi
            fi
        done

        if [ $test_count -eq 0 ]; then
            print_warning "No test executables found"
        elif [ $test_failed -eq 0 ]; then
            print_success "All $test_count tests passed!"
        else
            print_error "Some tests failed"
        fi
    fi
fi

# Return to original directory
cd "$ORIGINAL_DIR"

# Generate documentation if requested
if [ $BUILD_DOCS -eq 1 ]; then
    print_status "Generating documentation..."

    if [ ! -d "docs" ]; then
        mkdir -p docs
    fi

    if [ $CLEAN_DOCS -eq 1 ]; then
        rm -rf docs/html docs/latex
    fi

    if command_exists doxygen; then
        if [ -f "Doxyfile" ]; then
            doxygen Doxyfile
            if [ $? -eq 0 ]; then
                print_success "Documentation generated in docs/"
            else
                print_error "Documentation generation failed"
            fi
        else
            print_warning "Doxyfile not found, skipping documentation"
        fi
    else
        print_warning "Doxygen not found, skipping documentation"
    fi
fi

# Final success message
echo ""
echo -e "${BOLD}${GREEN}════════════════════════════════════════════${NC}"
echo -e "${BOLD}${GREEN}    Integrated Thread System Build Complete ${NC}"
echo -e "${BOLD}${GREEN}════════════════════════════════════════════${NC}"

if [ -d "build/bin" ]; then
    echo -e "${CYAN}Available executables:${NC}"
    ls -la build/bin/ 2>/dev/null || echo "  (no executables found)"
fi

echo ""
echo -e "${CYAN}Build Summary:${NC}"
echo -e "  Build Type: $BUILD_TYPE"
echo -e "  Target: $TARGET"
echo -e "  Compiler: $SELECTED_COMPILER_NAME"
echo -e "  Platform: $PLATFORM"
echo -e "  Build Time: $(date)"

if [ ${#sanitizers[@]} -gt 0 ]; then
    echo -e "  Sanitizers: ${sanitizers[*]}"
fi

if [ $BUILD_BENCHMARKS -eq 1 ]; then
    echo -e "  Benchmarks: Enabled"
fi

if [ $BUILD_DOCS -eq 1 ]; then
    echo -e "  Documentation: Generated"
fi

print_success "Build script completed successfully!"

exit 0