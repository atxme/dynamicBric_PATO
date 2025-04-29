#!/bin/bash
#=========================================================================
# Dynamic Bric PATO - Test Runner Script
# This script compiles and runs the test suite
#
# Author: Christophe Benedetti
#=========================================================================

set -e  # Exit immediately if a command exits with a non-zero status

# Terminal colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project paths
PROJECT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="build_test"
BUILD_PATH="$PROJECT_PATH/$BUILD_DIR"

# Build options
CMAKE_GENERATOR="Ninja"
BUILD_TYPE="Release"
ENABLE_TLS=OFF  # Set to ON to build with TLS support

# Print header
print_header() {
    echo -e "${BLUE}=================================================${NC}"
    echo -e "${BLUE}   Dynamic Bric PATO - Test Runner Script        ${NC}"
    echo -e "${BLUE}=================================================${NC}"
    echo
}

# Print section
print_section() {
    echo
    echo -e "${YELLOW}▶ $1${NC}"
    echo -e "${YELLOW}--------------------------------------------------${NC}"
}

# Start script
print_header

# Check if project directory exists
if [ ! -d "$PROJECT_PATH" ]; then
    echo -e "${RED}Error: Project directory does not exist: ${PROJECT_PATH}${NC}"
    exit 1
fi

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --tls-on)
            ENABLE_TLS=ON
            shift
            ;;
        --tls-off)
            ENABLE_TLS=OFF
            shift
            ;;
        --clean)
            print_section "Cleaning build directory"
            rm -rf "$BUILD_PATH"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --test=*)
            TEST_FILTER="${1#*=}"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --tls-on     Enable TLS support"
            echo "  --tls-off    Disable TLS support (default)"
            echo "  --clean      Clean build directory before building"
            echo "  --debug      Build in Debug mode"
            echo "  --test=NAME  Run only tests matching NAME"
            echo "  --help       Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Run '$0 --help' for usage information"
            exit 1
            ;;
    esac
done

# Create build directory if it doesn't exist
print_section "Setting up build directory"
mkdir -p "$BUILD_PATH"

# Change to build directory
cd "$BUILD_PATH"

# Configure with CMake
print_section "Configuring with CMake"
echo "Generator:  $CMAKE_GENERATOR"
echo "Build type: $BUILD_TYPE"
echo "TLS:        $ENABLE_TLS"

# Run CMake with the specified options
cmake -G "$CMAKE_GENERATOR" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DUSE_TLS="$ENABLE_TLS" \
      "$PROJECT_PATH/test"

# Compile
print_section "Compiling tests"
ninja

# Run tests
print_section "Running tests"
if [ -n "$TEST_FILTER" ]; then
    echo "Running tests matching: $TEST_FILTER"
    ./dynamicBric_PATO_test --gtest_filter="*${TEST_FILTER}*"
else
    ./dynamicBric_PATO_test
fi

# Check test status
if [ $? -eq 0 ]; then
    echo
    echo -e "${GREEN}✓ All tests passed${NC}"
else
    echo
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi

echo
echo -e "${GREEN}Test execution completed!${NC}"
echo

exit 0 