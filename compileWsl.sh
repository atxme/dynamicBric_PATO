#!/bin/bash
#=========================================================================
# Dynamic Bric PATO - WSL Compilation Script
# This script compiles the Dynamic Bric PATO project in WSL
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
PROJECT_PATH="$HOME/pato/dynamicBric_PATO"
BUILD_DIR="build"
BUILD_PATH="$PROJECT_PATH/$BUILD_DIR"

# Build options
CMAKE_GENERATOR="Ninja"
BUILD_TYPE="Release"  # Possible values: Debug, Release, RelWithDebInfo, MinSizeRel
ENABLE_TLS=OFF  # Set to ON to build with TLS support

# Print header
print_header() {
    echo -e "${BLUE}=================================================${NC}"
    echo -e "${BLUE}   Dynamic Bric PATO - WSL Compilation Script    ${NC}"
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
    echo -e "${YELLOW}Please ensure the source code is available at this location.${NC}"
    exit 1
fi

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
      ..

# Compile
print_section "Compiling"
ninja

# Check compilation status
if [ $? -eq 0 ]; then
    echo
    echo -e "${GREEN}✓ Compilation completed successfully${NC}"
    
    # Find the built library
    LIB_FILE=$(find . -name "*.so" -o -name "*.dll" | head -1)
    if [ -n "$LIB_FILE" ]; then
        LIB_SIZE=$(du -h "$LIB_FILE" | cut -f1)
        echo -e "${GREEN}✓ Library size: $LIB_SIZE ($LIB_FILE)${NC}"
    fi
else
    echo
    echo -e "${RED}✗ Compilation failed${NC}"
    exit 1
fi

# Run tests if they exist and compilation succeeded
if [ -x ./tests/run_tests ]; then
    print_section "Running tests"
    ./tests/run_tests
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ All tests passed${NC}"
    else
        echo -e "${RED}✗ Some tests failed${NC}"
        exit 1
    fi
fi

echo
echo -e "${GREEN}Build process completed!${NC}"
echo -e "${BLUE}You can find the build artifacts in: ${BUILD_PATH}${NC}"
echo

exit 0

