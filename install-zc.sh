#!/bin/bash

# zc Bundle Installer
# A cross-platform installer for zc and zc-kilo portable file manager
# Works on macOS and Linux systems

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[$(date '+%H:%M:%S')]${NC} $1"
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

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to detect OS
detect_os() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="linux"
    else
        print_error "Unsupported operating system: $OSTYPE"
        exit 1
    fi
}

# Function to check if required tools are available
check_requirements() {
    print_status "Checking system requirements..."
    
    if ! command_exists "make"; then
        print_error "make is required but not found. Please install make."
        exit 1
    fi
    
    if ! command_exists "gcc" && ! command_exists "clang"; then
        print_error "C compiler (gcc or clang) is required but not found."
        exit 1
    fi
    
    print_success "All required tools found"
}

# Function to build the bundle if it doesn't exist
build_bundle() {
    if [ ! -d "dist/zc-bundle" ]; then
        print_status "Building zc bundle..."
        make bundle
        if [ $? -ne 0 ]; then
            print_error "Failed to build zc bundle"
            exit 1
        fi
        print_success "Bundle built successfully"
    else
        print_success "Bundle already exists"
    fi
}

# Function to install the bundle
install_bundle() {
    local install_dir="./zc-bundle-install"
    
    # Create installation directory
    mkdir -p "$install_dir"
    
    # Copy the bundle
    print_status "Installing zc bundle to $install_dir..."
    cp -r dist/zc-bundle/* "$install_dir/"
    
    # Make executables executable
    chmod +x "$install_dir/zc"
    chmod +x "$install_dir/zc-kilo"
    
    print_success "Installation completed successfully"
    echo ""
    echo "Installation details:"
    echo "  - Bundle installed to: $install_dir"
    echo "  - zc executable: $install_dir/zc"
    echo "  - zc-kilo executable: $install_dir/zc-kilo"
    echo ""
    
    # Show version info
    echo "Version information:"
    "$install_dir/zc" --version
}

# Function to show usage
show_usage() {
    echo "zc Bundle Installer"
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -v, --version  Show version information"
    echo "  -f, --force    Force reinstallation"
    echo ""
    echo "Examples:"
    echo "  $0                    # Install zc bundle"
    echo "  $0 --force            # Force reinstall"
    echo ""
}

# Function to show version
show_version() {
    echo "zc Bundle Installer v1.0"
    echo "This installer sets up the zc portable file manager"
}

# Main installation function
main() {
    local force_install=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -v|--version)
                show_version
                exit 0
                ;;
            -f|--force)
                force_install=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Print welcome message
    echo "=============================================="
    echo "zc Bundle Installer"
    echo "Portable file manager for macOS and Linux"
    echo "=============================================="
    echo ""
    
    # Detect OS
    detect_os
    print_status "Detected OS: $OS"
    
    # Check requirements
    check_requirements
    
    # Build bundle if needed
    if [ "$force_install" = true ] && [ -d "dist/zc-bundle" ]; then
        print_status "Force reinstall requested, rebuilding bundle..."
        make clean
        make bundle
    else
        build_bundle
    fi
    
    # Install bundle
    install_bundle
    
    # Show final instructions
    echo ""
    echo "=============================================="
    print_success "Installation completed successfully!"
    echo "=============================================="
    echo ""
    echo "To use zc, you can now run:"
    echo "  ./zc-bundle-install/zc       # Launch the file manager"
    echo "  ./zc-bundle-install/zc-kilo  # Launch the text editor"
    echo ""
    echo "For more information, run:"
    echo "  ./zc-bundle-install/zc --help"
    echo ""
}

# Run main function with all arguments
main "$@"
