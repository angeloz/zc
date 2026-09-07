# zc Bundle Installation Guide

This document provides instructions for installing the zc portable file manager bundle on macOS and Linux systems.

## What is zc?

zc (zero-commander) is a portable two-panel file commander that works without installation. It's designed to be distributed as a bundle containing all necessary components.

## Installation Script

We provide an installation script that handles the setup process for both macOS and Linux systems.

### Prerequisites

- macOS or Linux system
- Make tool installed
- C compiler (gcc or clang)
- Git (optional, for cloning the repository)

### Installation Steps

1. **Clone or download the repository**:
   ```bash
   git clone <repository-url>
   cd <repository-name>
   ```

2. **Run the installation script**:
   ```bash
   chmod +x ./install-zc.sh
   ./install-zc.sh
   ```

3. **Use the installed bundle**:
   ```bash
   ./zc-bundle-install/zc        # Launch the file manager
   ./zc-bundle-install/zc-kilo  # Launch the text editor
   ```

### Installation Options

- `--help` or `-h`: Show help information
- `--version` or `-v`: Show version information
- `--force` or `-f`: Force reinstallation (rebuilds the bundle)

### Installation Location

The installation script creates a directory called `zc-bundle-install` in the current directory containing:
- `zc`: The main file manager executable
- `zc-kilo`: The text editor executable
- `tools/`: Directory for optional helper tools such as `restic`

## Usage

After installation, you can use zc as a portable file manager:

```bash
# Launch the file manager
./zc-bundle-install/zc

# View help
./zc-bundle-install/zc --help

# Check version
./zc-bundle-install/zc --version
```

## Platform Support

The installation script supports both macOS and Linux systems. The bundle is built using the native toolchain for each platform.

## Troubleshooting

If you encounter issues:

1. **Permission errors**: Make sure you have write permissions in the directory where you're running the script
2. **Missing dependencies**: Install make and a C compiler if they're not available
3. **Terminal issues**: The text editor (zc-kilo) may have issues in some terminal environments

## Uninstalling

To remove the installation:

```bash
rm -rf ./zc-bundle-install
```

## Bundle Structure

The zc bundle contains:
- `zc`: Main file manager application
- `zc-kilo`: Text editor based on the kilo editor
- `tools/`: Directory for optional helper tools such as `restic`

## Development Notes

The installation script builds the bundle using the Makefile in the project root. The bundle includes:
- The main zc application
- The zc-kilo text editor (from third_party/kilo)
- All necessary dependencies for portable execution

The bundle is designed to be self-contained and work without requiring installation or system-wide dependencies.
