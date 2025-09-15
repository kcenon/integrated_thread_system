#!/bin/bash

# Integrated Thread System Dependency Setup Script
# Installs and configures all required dependencies for building the integrated system

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
echo -e "${BOLD}${BLUE}    Integrated Thread System Dependencies      ${NC}"
echo -e "${BOLD}${BLUE}    Automatic Setup and Installation           ${NC}"
echo -e "${BOLD}${BLUE}================================================${NC}"

# Function to print messages
print_status() { echo -e "${BOLD}${BLUE}[STATUS]${NC} $1"; }
print_success() { echo -e "${BOLD}${GREEN}[SUCCESS]${NC} $1"; }
print_error() { echo -e "${BOLD}${RED}[ERROR]${NC} $1"; }
print_warning() { echo -e "${BOLD}${YELLOW}[WARNING]${NC} $1"; }
print_info() { echo -e "${BOLD}${CYAN}[INFO]${NC} $1"; }

# Function to check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Function to detect platform
detect_platform() {
    case "$(uname -s)" in
        Darwin)
            PLATFORM="macOS"
            PLATFORM_ID="darwin"
            ;;
        Linux)
            PLATFORM="Linux"
            PLATFORM_ID="linux"

            # Detect Linux distribution
            if [ -f /etc/os-release ]; then
                . /etc/os-release
                DISTRO="$ID"
                DISTRO_VERSION="$VERSION_ID"
            elif command_exists lsb_release; then
                DISTRO=$(lsb_release -si | tr '[:upper:]' '[:lower:]')
                DISTRO_VERSION=$(lsb_release -sr)
            else
                DISTRO="unknown"
                DISTRO_VERSION="unknown"
            fi
            ;;
        MINGW*|CYGWIN*|MSYS*)
            PLATFORM="Windows"
            PLATFORM_ID="windows"
            ;;
        *)
            PLATFORM="Unknown"
            PLATFORM_ID="unknown"
            ;;
    esac

    print_info "Detected platform: $PLATFORM"
    if [ "$PLATFORM_ID" = "linux" ]; then
        print_info "Linux distribution: $DISTRO $DISTRO_VERSION"
    fi
}

# Function to install system dependencies
install_system_dependencies() {
    print_status "Installing system dependencies..."

    case "$PLATFORM_ID" in
        linux)
            case "$DISTRO" in
                ubuntu|debian)
                    print_status "Installing dependencies for Ubuntu/Debian..."

                    # Update package list
                    sudo apt-get update

                    # Essential build tools
                    sudo apt-get install -y \
                        build-essential \
                        cmake \
                        git \
                        pkg-config \
                        curl \
                        zip \
                        unzip \
                        tar

                    # C++ compilers and tools
                    sudo apt-get install -y \
                        g++ \
                        clang \
                        ninja-build \
                        ccache

                    # Libraries that might be needed
                    sudo apt-get install -y \
                        libc6-dev \
                        libstdc++-dev \
                        linux-libc-dev

                    # Optional but useful
                    sudo apt-get install -y \
                        doxygen \
                        graphviz \
                        valgrind \
                        gdb \
                        htop
                    ;;

                centos|rhel|rocky|almalinux)
                    print_status "Installing dependencies for CentOS/RHEL/Rocky/AlmaLinux..."

                    # Enable EPEL if needed
                    if ! sudo yum repolist | grep -q epel; then
                        sudo yum install -y epel-release
                    fi

                    # Development tools
                    sudo yum groupinstall -y "Development Tools"
                    sudo yum install -y \
                        cmake3 \
                        git \
                        pkg-config \
                        curl \
                        zip \
                        unzip

                    # Create cmake symlink if needed
                    if command_exists cmake3 && ! command_exists cmake; then
                        sudo ln -sf /usr/bin/cmake3 /usr/bin/cmake
                    fi

                    # Additional tools
                    sudo yum install -y \
                        gcc-c++ \
                        clang \
                        ninja-build \
                        ccache \
                        doxygen \
                        graphviz \
                        valgrind \
                        gdb
                    ;;

                fedora)
                    print_status "Installing dependencies for Fedora..."

                    sudo dnf install -y \
                        gcc-c++ \
                        cmake \
                        git \
                        pkg-config \
                        curl \
                        zip \
                        unzip \
                        ninja-build \
                        clang \
                        ccache \
                        doxygen \
                        graphviz \
                        valgrind \
                        gdb
                    ;;

                opensuse*)
                    print_status "Installing dependencies for openSUSE..."

                    sudo zypper install -y \
                        gcc-c++ \
                        cmake \
                        git \
                        pkg-config \
                        curl \
                        zip \
                        unzip \
                        ninja \
                        clang \
                        ccache \
                        doxygen \
                        graphviz \
                        valgrind \
                        gdb
                    ;;

                arch)
                    print_status "Installing dependencies for Arch Linux..."

                    sudo pacman -Syu --noconfirm \
                        base-devel \
                        cmake \
                        git \
                        pkg-config \
                        curl \
                        zip \
                        unzip \
                        ninja \
                        clang \
                        ccache \
                        doxygen \
                        graphviz \
                        valgrind \
                        gdb
                    ;;

                alpine)
                    print_status "Installing dependencies for Alpine Linux..."

                    sudo apk update
                    sudo apk add \
                        build-base \
                        cmake \
                        git \
                        pkgconfig \
                        curl \
                        zip \
                        unzip \
                        ninja \
                        clang \
                        ccache \
                        doxygen \
                        graphviz \
                        valgrind \
                        gdb
                    ;;

                *)
                    print_warning "Unknown Linux distribution: $DISTRO"
                    print_info "Please install the following packages manually:"
                    print_info "  build-essential, cmake, git, pkg-config, curl, zip, unzip"
                    print_info "  g++, clang, ninja-build, ccache, doxygen"
                    ;;
            esac
            ;;

        darwin)
            print_status "Installing dependencies for macOS..."

            # Check for Homebrew
            if ! command_exists brew; then
                print_status "Installing Homebrew..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

                # Add Homebrew to PATH for current session
                if [[ -f "/opt/homebrew/bin/brew" ]]; then
                    eval "$(/opt/homebrew/bin/brew shellenv)"
                elif [[ -f "/usr/local/bin/brew" ]]; then
                    eval "$(/usr/local/bin/brew shellenv)"
                fi
            fi

            # Update Homebrew
            brew update

            # Install essential tools
            brew install \
                cmake \
                git \
                pkg-config \
                curl \
                ninja \
                ccache \
                doxygen \
                graphviz

            # Install compilers (if not already present)
            if ! command_exists gcc; then
                brew install gcc
            fi

            if ! command_exists clang; then
                # Clang usually comes with Xcode command line tools
                if ! xcode-select -p &>/dev/null; then
                    print_status "Installing Xcode Command Line Tools..."
                    xcode-select --install
                fi
            fi

            # Optional tools
            brew install --quiet \
                llvm \
                valgrind \
                htop \
                2>/dev/null || true
            ;;

        windows)
            print_status "Windows environment detected..."
            print_info "For Windows, please use one of the following:"
            print_info "  1. Visual Studio with C++ workload"
            print_info "  2. MSYS2/MinGW64"
            print_info "  3. WSL2 with Ubuntu"
            print_info ""
            print_info "This script can continue with MSYS2 if available..."

            if command_exists pacman; then
                print_status "MSYS2 detected, installing dependencies..."
                pacman -S --noconfirm \
                    mingw-w64-x86_64-gcc \
                    mingw-w64-x86_64-cmake \
                    mingw-w64-x86_64-ninja \
                    mingw-w64-x86_64-pkg-config \
                    git \
                    unzip \
                    zip
            else
                print_warning "Please install dependencies manually for Windows"
                return 1
            fi
            ;;

        *)
            print_error "Unsupported platform: $PLATFORM"
            return 1
            ;;
    esac

    print_success "System dependencies installed"
}

# Function to setup vcpkg
setup_vcpkg() {
    print_status "Setting up vcpkg for dependency management..."

    local vcpkg_dir="../vcpkg"

    if [ -d "$vcpkg_dir" ]; then
        print_info "vcpkg directory already exists"
        cd "$vcpkg_dir"

        # Update existing vcpkg
        print_status "Updating vcpkg..."
        git pull origin master

    else
        print_status "Cloning vcpkg repository..."
        cd ..
        git clone https://github.com/Microsoft/vcpkg.git
        cd vcpkg
    fi

    # Bootstrap vcpkg
    print_status "Bootstrapping vcpkg..."
    case "$PLATFORM_ID" in
        windows)
            if [ -f "bootstrap-vcpkg.bat" ]; then
                ./bootstrap-vcpkg.bat
            else
                print_error "vcpkg bootstrap script not found"
                return 1
            fi
            ;;
        *)
            if [ -f "bootstrap-vcpkg.sh" ]; then
                ./bootstrap-vcpkg.sh
            else
                print_error "vcpkg bootstrap script not found"
                return 1
            fi
            ;;
    esac

    if [ $? -ne 0 ]; then
        print_error "vcpkg bootstrap failed"
        return 1
    fi

    # Test vcpkg
    print_status "Testing vcpkg installation..."
    if ./vcpkg version >/dev/null 2>&1; then
        print_success "vcpkg is working correctly"
    else
        print_error "vcpkg test failed"
        return 1
    fi

    # Install common dependencies
    print_status "Installing common C++ libraries via vcpkg..."
    local packages=(
        "fmt"
        "spdlog"
        "catch2"
        "benchmark"
        "nlohmann-json"
    )

    for package in "${packages[@]}"; do
        print_info "Installing $package..."
        if ! ./vcpkg install "$package"; then
            print_warning "Failed to install $package, continuing..."
        fi
    done

    # Return to original directory
    cd - >/dev/null

    print_success "vcpkg setup completed"
}

# Function to verify installation
verify_installation() {
    print_status "Verifying installation..."

    local missing_tools=()
    local working_tools=()

    # Check essential tools
    local tools=(
        "cmake:CMake build system"
        "git:Git version control"
        "pkg-config:Package configuration"
    )

    for tool_info in "${tools[@]}"; do
        local tool="${tool_info%%:*}"
        local description="${tool_info##*:}"

        if command_exists "$tool"; then
            working_tools+=("$tool ($description)")
        else
            missing_tools+=("$tool ($description)")
        fi
    done

    # Check compilers
    local compilers=()
    if command_exists g++; then
        local version=$(g++ --version 2>/dev/null | head -n1)
        compilers+=("GCC: $version")
    fi

    if command_exists clang++; then
        local version=$(clang++ --version 2>/dev/null | head -n1)
        compilers+=("Clang: $version")
    fi

    # Check build systems
    local build_systems=()
    if command_exists make; then
        build_systems+=("Make")
    fi

    if command_exists ninja; then
        build_systems+=("Ninja")
    fi

    # Check vcpkg
    local vcpkg_status="Not available"
    if [ -f "../vcpkg/vcpkg" ] || [ -f "../vcpkg/vcpkg.exe" ]; then
        if ../vcpkg/vcpkg version >/dev/null 2>&1; then
            vcpkg_status="Working"
        else
            vcpkg_status="Installed but not working"
        fi
    fi

    # Display results
    echo ""
    echo -e "${BOLD}${CYAN}Installation Verification Results:${NC}"
    echo -e "${BOLD}${CYAN}══════════════════════════════════${NC}"

    if [ ${#working_tools[@]} -gt 0 ]; then
        echo -e "${BOLD}${GREEN}✓ Working Tools:${NC}"
        for tool in "${working_tools[@]}"; do
            echo -e "  • $tool"
        done
        echo ""
    fi

    if [ ${#missing_tools[@]} -gt 0 ]; then
        echo -e "${BOLD}${RED}✗ Missing Tools:${NC}"
        for tool in "${missing_tools[@]}"; do
            echo -e "  • $tool"
        done
        echo ""
    fi

    if [ ${#compilers[@]} -gt 0 ]; then
        echo -e "${BOLD}${GREEN}✓ Available Compilers:${NC}"
        for compiler in "${compilers[@]}"; do
            echo -e "  • $compiler"
        done
        echo ""
    else
        echo -e "${BOLD}${RED}✗ No C++ compilers found${NC}"
        echo ""
    fi

    if [ ${#build_systems[@]} -gt 0 ]; then
        echo -e "${BOLD}${GREEN}✓ Build Systems:${NC}"
        for bs in "${build_systems[@]}"; do
            echo -e "  • $bs"
        done
        echo ""
    else
        echo -e "${BOLD}${RED}✗ No build systems found${NC}"
        echo ""
    fi

    echo -e "${BOLD}${CYAN}vcpkg Status:${NC} $vcpkg_status"
    echo ""

    # Overall status
    if [ ${#missing_tools[@]} -eq 0 ] && [ ${#compilers[@]} -gt 0 ] && [ ${#build_systems[@]} -gt 0 ]; then
        print_success "All essential dependencies are available!"

        if [ "$vcpkg_status" = "Working" ]; then
            print_success "vcpkg is ready for advanced dependency management"
        fi

        return 0
    else
        print_error "Some essential dependencies are missing"
        return 1
    fi
}

# Function to create configuration file
create_config_file() {
    print_status "Creating build configuration file..."

    cat > build_config.sh << 'EOF'
#!/bin/bash
# Integrated Thread System Build Configuration
# Generated by dependency.sh

# Compiler preferences (can be overridden by build.sh options)
export PREFERRED_CXX_COMPILER=""  # Auto-detect
export PREFERRED_C_COMPILER=""    # Auto-detect

# Build preferences
export DEFAULT_BUILD_TYPE="Release"
export DEFAULT_BUILD_CORES="auto"

# Feature flags
export ENABLE_VCPKG="auto"
export ENABLE_SANITIZERS="off"
export ENABLE_BENCHMARKS="off"

# Platform-specific settings
case "$(uname -s)" in
    Darwin)
        export MACOS_BUILD_CORES_LIMIT="8"  # Avoid memory pressure
        ;;
    Linux)
        if [ "$(uname -m)" == "aarch64" ]; then
            export VCPKG_FORCE_SYSTEM_BINARIES="arm"
        fi
        ;;
esac

# Integration settings
export THREAD_SYSTEM_PATH="../thread_system"
export LOGGER_SYSTEM_PATH="../logger_system"
export MONITOR_SYSTEM_PATH="../monitoring_system"

echo "Build configuration loaded for $(uname -s)"
EOF

    chmod +x build_config.sh
    print_success "Build configuration file created: build_config.sh"
}

# Function to show usage help
show_help() {
    echo -e "${BOLD}Usage:${NC} $0 [options]"
    echo ""
    echo -e "${BOLD}Options:${NC}"
    echo "  --skip-system     Skip system dependency installation"
    echo "  --skip-vcpkg      Skip vcpkg setup"
    echo "  --verify-only     Only verify existing installation"
    echo "  --clean-vcpkg     Clean and reinstall vcpkg"
    echo "  --help            Show this help message"
    echo ""
    echo -e "${BOLD}Examples:${NC}"
    echo "  $0                    # Full installation"
    echo "  $0 --skip-system     # Skip system packages, only setup vcpkg"
    echo "  $0 --verify-only     # Check what's already installed"
    echo ""
}

# Parse command line arguments
SKIP_SYSTEM=0
SKIP_VCPKG=0
VERIFY_ONLY=0
CLEAN_VCPKG=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-system)
            SKIP_SYSTEM=1
            shift
            ;;
        --skip-vcpkg)
            SKIP_VCPKG=1
            shift
            ;;
        --verify-only)
            VERIFY_ONLY=1
            shift
            ;;
        --clean-vcpkg)
            CLEAN_VCPKG=1
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

# Main execution
print_info "Starting dependency setup for Integrated Thread System"
echo ""

# Detect platform
detect_platform
echo ""

# Verification only mode
if [ $VERIFY_ONLY -eq 1 ]; then
    verify_installation
    exit $?
fi

# Clean vcpkg if requested
if [ $CLEAN_VCPKG -eq 1 ] && [ -d "../vcpkg" ]; then
    print_status "Cleaning existing vcpkg installation..."
    rm -rf "../vcpkg"
fi

# Install system dependencies
if [ $SKIP_SYSTEM -eq 0 ]; then
    install_system_dependencies
    if [ $? -ne 0 ]; then
        print_error "System dependency installation failed"
        exit 1
    fi
    echo ""
fi

# Setup vcpkg
if [ $SKIP_VCPKG -eq 0 ]; then
    setup_vcpkg
    if [ $? -ne 0 ]; then
        print_error "vcpkg setup failed"
        exit 1
    fi
    echo ""
fi

# Create configuration file
create_config_file
echo ""

# Verify installation
verify_installation
result=$?

echo ""
if [ $result -eq 0 ]; then
    echo -e "${BOLD}${GREEN}════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${GREEN}    Dependency Setup Completed Successfully  ${NC}"
    echo -e "${BOLD}${GREEN}════════════════════════════════════════════${NC}"
    echo ""
    print_success "You can now run './build.sh' to build the Integrated Thread System"
    echo ""
    print_info "Quick start commands:"
    print_info "  ./build.sh              # Interactive build with compiler selection"
    print_info "  ./build.sh --auto       # Automatic build with best compiler"
    print_info "  ./build.sh --help       # Show all build options"
else
    echo -e "${BOLD}${YELLOW}════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${YELLOW}    Dependency Setup Completed with Issues  ${NC}"
    echo -e "${BOLD}${YELLOW}════════════════════════════════════════════${NC}"
    echo ""
    print_warning "Some dependencies are missing, but you can still try building"
    print_info "Run './build.sh --help' for alternative build options"
fi

exit $result